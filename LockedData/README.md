`LockedData`
------------

`LockedData` contains an abstraction that I implemented in my OS class that I
find useful in multi threaded programming.  Essentially this library helps you
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

With minimal to no loss of efficiency.  Of course this should be set to fit
different concurrency patterns.  For example, it should be designed to be fit
to use with monitors.

### The interface
