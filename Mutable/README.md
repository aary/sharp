`Mutable` A library for mutability
---------

A library utility for mutability, this has several advantages over mutability
from the langauge level, which is not built into the type system
(distinguishing it from specifiers like `const` and `volatile`).  This leads
to some limitations.  For example

```c++
auto mutexes = std::unordered_map<int, std::mutex>{};
// fill map

// read thread, cannot modify the map
void foo(const std::unordered_map<int, std::mutex>& mutexes) {
    auto& mutex = mutexes.at(1);
    mutex.lock(); // error
}
```

The above does not work because there is no way to enforce mutability into the
type system and have it carry through a library's API.  The solution to the
above is simple

```c++
auto mutexes = std::unordered_map<int, sharp::Mutable<std::mutex>>{};

// read thread, cannot modify the map
void foo(const std::unordered_map<int, std::mutex>& mutexes) {
    auto& mutex = mutexes.at(1);
    mutex->lock();
}
```

This carries through the type system and enforces mutability right at the
source wherever it is required

Once the utility is built into the type system, it also has other uses.  For
example, it can also help in template metaprogramming when you want to
optionally include functionality and state into a class without impacting its
size and functionality when it's not needed as a base class

```c++
template <typename Type>
class Something : public sharp::Mutable<Base<Something>> { ... };
```
