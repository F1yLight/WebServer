#ifndef LOCKER_H
#include "../inc/Locker.h"
#include <exception>
#include <iostream>

Locker::Locker() {
    if (pthread_mutex_init(&m_mutex, NULL) != 0)
        throw std::runtime_error("Failed to initialize mutex");
}

Locker::~Locker() { pthread_mutex_destroy(&m_mutex); }

bool Locker::lock() { return pthread_mutex_lock(&m_mutex) == 0; }

bool Locker::unlock() { return pthread_mutex_unlock(&m_mutex) == 0; }

#endif