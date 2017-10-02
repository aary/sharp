`Concurrent`
------------

`Concurrent` contains an abstraction that I implemented in my OS class that I
find useful in concurrent programming.  Essentially this library helps you
write concurrent code without some of the drawbacks of free mutexes and
condition variables.  Mutexes are logically associated with the data they
protect, condition variables are associated with the threads they block given
a certain condition.  This class tries to package both of those into one
simple bite sized concurrency primitive

This is what you would do if you wanted to have three data items that could
potentially be accessed concurrently from different threads

```c++
    std::mutex mtx_vector;
    std::vector<int> critical_vector;
    std::mutex mtx_deque;
    std::deque<int> critical_deque;
    std::mutex mtx_map
    std::unordered_map<int, int> information_map;
```

This is ugly, and it is not clear which mutex is meant to protect which data
item.  Throw in some condition variables in there and the code becomes
chaotic, hard to read and hard to reason about at first glance (sometimes even
at tenth glance)

Instead you could have something like the following

```c++
    sharp::Concurrent<std::vector<int>> vector;
    sharp::Concurrent<std::deque<int>> deque;
    sharp::Concurrent<std::unordered_map<int, int>> map;
```

This makes the semantics of the code clear and self documenting - There are
three data items that will possibly be accessed from different threads
concurrently

### The interface

The simplest use case of this library is to execute code that will be
synchronized off an internal mutex associated with the data item.  This can be
done nicely with lambdas

```c++
auto vec = sharp::Concurrent<std::vector<int>>{};
auto size = vec.atomic([](auto& vec) { return v.size(); });
```

This executes the size read on the vector synchronously off a mutex associated
with the vector internally.  The library manages that for you

The library also provides a simple RAII managed locking mechanism that avoids
having to rely on other external RAII wrappers like `std::unique_lock` and
`std::lock_guard` for just locking and unlocking a mutex.  The interface
handles that for you

```c++
auto lock = vec.lock();
lock->push_back(1);
lock->push_back(2);
lock->pop_back();
for (auto integer : *lock) {
    cout << integer << endl;
}
lock.unlock();
```

This avoids the problem of having to construct RAII wrappers around mutex
classes for better more exception safe code.  There is no way to lock the
mutex without being robust in the face of exceptions

Further the library optimizes contention when the program is written in a
const correct manner.  When the concurrent object is const, a `lock()` call
tries to acquire a shared lock instead of an exclusive unique lock, this helps
increase throughput in high read scenarios for reader threads

Conditional critical sections (inspired by [Google's Abseil Mutex
class](https://goo.gl/JhhGFp)) are also supported.  The interface here tries
to eliminate some of the drawbacks of condition variables (for example -
manual signalling, broadcasting, coarse lock contention, forgetting to signal,
etc.)

```c++
// thread 1
auto lock = data.lock();
lock.wait([](auto& data) {
    return data.is_ready();
});
cout << data.get() << endl;

// thread 2
auto lock = data.lock();
lock->set_value(4);
lock.unlock();
```
