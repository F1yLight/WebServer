#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <mutex>

class Locker {
  private:
    pthread_mutex_t m_mutex;
    
  public:
    Locker();

    ~Locker();

    bool lock();

    bool unlock();
};

#endif