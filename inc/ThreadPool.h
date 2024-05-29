#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "Locker.h"
#include "Semaphore.h"
#include <iostream>
#include <list>
#include <pthread.h>


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~类定义~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

template <typename T> class ThreadPool {
  private:
    int max_pthread;           // 线程池中的最大线程总数
    int max_task;              // 工作队列的最大总数
    pthread_t *pthread_poll;   // 线程池数组
    std::list<T *> task_queue; // 请求队列
    Locker locker;             // 保护请求队列的互斥锁
    Semphore task_sem; // 由信号量来判断是否有任务需要处理
    bool flag_stop;    // 是否结束线程

    // 线程的工作函数，静态成员函数
    static void *worker(void *arg);
    void run();

  public:
    ThreadPool();
    ~ThreadPool();
    bool add_task(T *request);
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~函数定义~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// 私有函数

template <typename T> void ThreadPool<T>::run() {
    while (!flag_stop) {
        task_sem.wait();
        locker.lock();
        if (task_queue.empty()) {
            locker.unlock();
            continue;
        }
        T *request = task_queue.front();
        task_queue.pop_front();
        locker.unlock();
        if (!request)
            continue;
        request->doit();
    }
}

template <typename T> void *ThreadPool<T>::worker(void *arg) {
    ThreadPool *pool = (ThreadPool *)arg;
    pool->run();
    return pool;
}

// 公共函数

template <typename T> ThreadPool<T>::ThreadPool() {
    max_pthread = 8;
    max_task = 1024;
    flag_stop = false;
    pthread_poll = new pthread_t[max_pthread];
    if (!pthread_poll)
        throw std::exception();
    for (int i = 0; i < max_pthread; i++) {
        // std::cout << "Create the pthread:" << i << '\n';
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

template <typename T> ThreadPool<T>::~ThreadPool() {
    delete[] pthread_poll;
    flag_stop = true;
}

template <typename T> bool ThreadPool<T>::add_task(T *request) {
    locker.lock();

    // 如果请求队列大于了最大请求队列，则出错
    if ((int)task_queue.size() >= max_task) {
        locker.unlock();
        return false;
    }
    task_queue.push_back(request); // 将请求加入到请求队列中
    locker.unlock();
    task_sem.post(); // 将信号量增加1
    return true;
}

#endif