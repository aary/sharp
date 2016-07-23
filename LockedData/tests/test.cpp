#include "LockedData.hpp"
#include <cassert>
#include <iostream>
#include <mutex>
using namespace std;

struct shared_mutex {
    void lock_shared() {}
    void lock() {}
    void unlock() {}
};

struct Base {};
struct Inherited : public Base {};

void func(Base&) {
    cout << "func(Base&)" << endl;
}
void func(Inherited&) {
    cout << "func(Inherited&)" << endl;
}

int main() {
    cout << "Hello world" << endl;

    Base b;
    Inherited in;
    func(b);
    func(in);

    std::mutex mtx;
    shared_mutex mtx_shared;
    sharp::detail::lock_mutex<std::mutex>(mtx, sharp::detail::ReadLockTag{});
    sharp::detail::lock_mutex<shared_mutex>(
            mtx_shared,
            sharp::detail::ReadLockTag{});

    return 0;
}
