`LockedData`
------------

`LockedData` contains an abstraction that I implemented in my OS class that I
find useful in concurrent programming.  Essentially this library helps you
write concurrent code without having the need to maintain a separate mutex for
every data object that is shared across multiple threads.  For example the
following code could get complicated very fast

``` Cpp
    std::mutex mtx_vector;
    std::vector<int> critical_vector;
    std::mutex mtx_deque;
    std::deque<int> critical_deque;
    std::mutex mtx_map
    std::unordered_map<int, int> information_map;
```

Instead you could have something like the following

``` Cpp
    LockedData<std::vector<int>> critical_vector;
    LockedData<std::deque<int>> critical_deque;
    LockedData<std::unordered_map<int, int> information_map;
```

With minimal to no loss of efficiency.  Of course this should be designed to
fit different concurrency patterns.  For example, it should be designed to be
usable when using monitors so the internal mutex is made available in those
cases

### The interface

#### Simple critical sections
```c++
auto vec_locked = LockedData<std::vector<int>>{};
// ...
auto& ele = vec_locked.execute_atomic([&](auto& v) { return v[0]; });
```

#### Lock proxy like `std::weak_ptr`
```c++
auto handle = vec_locked.lock();
handle->push_back(1);
handle->push_back(2);
handle->pop_back();
for (auto integer : *handle) {
    cout << integer << endl;
}
```

#### Monitors
```c++
auto handle = vec_locked.lock();
while (some_condition()) {
    cv.wait(vec_locked.get_unique_lock());
}
```
