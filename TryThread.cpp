#ifndef _THREADPOOL_H
#define _THREADPOOL_H
// #include "myhttp_coon.h"
// #include "mylock.h"
#include <cstdio>
#include <exception>
#include <iostream>
#include <list>
#include <pthread.h>
#include <semaphore.h>
using namespace std;

// mylocker 类的实现
class mylocker {
  private:
    pthread_mutex_t m_mutex;

  public:
    mylocker() {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
            throw std::exception();
    }

    ~mylocker() { pthread_mutex_destroy(&m_mutex); }

    bool lock() { return pthread_mutex_lock(&m_mutex) == 0; }

    bool unlock() { return pthread_mutex_unlock(&m_mutex) == 0; }
};

// sem 类的实现
class sem {
  private:
    sem_t m_sem;

  public:
    sem() {
        if (sem_init(&m_sem, 0, 0) != 0)
            throw std::exception();
    }

    sem(int num) {
        if (sem_init(&m_sem, 0, num) != 0)
            throw std::exception();
    }

    ~sem() { sem_destroy(&m_sem); }

    bool wait() { return sem_wait(&m_sem) == 0; }

    bool post() { return sem_post(&m_sem) == 0; }
};

template <typename T>
/*线程池的封装*/
class threadpool {
  private:
    int max_thread;               // 线程池中的最大线程总数
    int max_job;                  // 工作队列的最大总数
    pthread_t *pthread_poll;      // 线程池数组
    std::list<T *> m_myworkqueue; // 请求队列
    mylocker m_queuelocker;       // 保护请求队列的互斥锁
    sem m_queuestat; // 由信号量来判断是否有任务需要处理
    bool m_stop;     // 是否结束线程
  public:
    threadpool();
    ~threadpool();
    bool addjob(T *request);

  private:
    static void *worker(void *arg);
    void run();
};

template <typename T> void *threadpool<T>::worker(void *arg) {
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

/*线程池的创建*/
template <typename T> threadpool<T>::threadpool() {
    max_thread = 8;
    max_job = 1000;
    m_stop = false;
    pthread_poll = new pthread_t[max_thread]; // 为线程池开辟空间
    if (!pthread_poll)
        throw std::exception();
    for (int i = 0; i < max_thread; i++) {
        cout << "Create the pthread:" << i << endl;
        if (pthread_create(pthread_poll + i, NULL, worker, this) != 0) {
            delete[] pthread_poll;
            throw std::exception();
        }

        // 将线程分离，这意味着当线程结束后，资源会自动回收。如果分离失败，则抛出异常
        if (pthread_detach(pthread_poll[i])) {
            delete[] pthread_poll;
            throw std::exception();
        }
    }
}

template <typename T> threadpool<T>::~threadpool() {
    delete[] pthread_poll;
    m_stop = true;
}

template <typename T> bool threadpool<T>::addjob(T *request) {
    m_queuelocker.lock();

    // 如果请求队列大于了最大请求队列，则出错
    if (m_myworkqueue.size() > max_job) {
        m_queuelocker.unlock();
        return false;
    }
    m_myworkqueue.push_back(request); // 将请求加入到请求队列中
    m_queuelocker.unlock();
    m_queuestat.post(); // 将信号量增加1
    return true;
}

template <typename T> void threadpool<T>::run() {
    while (!m_stop) {
        m_queuestat.wait(); // 信号量减1，直到为0的时候线程挂起等待
        m_queuelocker.lock();
        if (m_myworkqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_myworkqueue.front();
        m_myworkqueue.pop_front();
        m_queuelocker.unlock();
        if (!request)
            continue;
        request->doit(); // 执行工作队列
    }
}
#endif

int main() {
    cout << 1 << '\n';
    return 0;
}