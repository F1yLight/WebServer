#include "../inc/Locker.h"
#include "../inc/Semaphore.h"
#include "../inc/ThreadPool.h"
#include <iostream>
#include <unistd.h>

class poi {
  public:
    int x;

    poi(): x(1) {}
    void doit() {
        std::cout << x++ << '\n';
    }
};

void sol() {
    Locker locker;
    Semphore sem = Semphore(10);
    ThreadPool<poi> pool;
    for (int i = 0; i < 10; i++) {
        pool.add_task(new poi());
    }
    sleep(0.5);
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(0);
    sol();
    return 0;
}