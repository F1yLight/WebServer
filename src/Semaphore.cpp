#ifndef SEMAPHORE_H

#include "../inc/Semaphore.h"
#include <iostream>

Semphore::Semphore() {
    if (sem_init(&sem, 0, 0) != 0)
        throw std::runtime_error("Failed to initialize semaphore");
}

Semphore::Semphore(int num) {
    if (sem_init(&sem, 0, num) != 0)
        throw std::runtime_error("Failed to initialize semaphore with value " + std::to_string(num));
}

Semphore::~Semphore() { sem_destroy(&sem); }

bool Semphore::wait() { return sem_wait(&sem) == 0; }

bool Semphore::post() { return sem_post(&sem) == 0; }

#endif