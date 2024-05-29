#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <semaphore.h>

class Semphore {
  private:
    sem_t sem;

  public:
    Semphore();

    Semphore(int num);

    ~Semphore();

    bool wait();

    bool post();
};

#endif