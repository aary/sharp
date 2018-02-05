`Mutex`
---------

Very *very* small mutexes for fine grained locking

```c++
auto mutex = sharp::Mutex<>{};

mutex.lock();
...
mutex.unlock();
```

This module contains two locks - a spinlock (`sharp::Spinlock`) and a full
featured adaptive sleeping lock (`sharp::Mutex`).  Both locks take up one
bit and two bits respectively.

C++ requires that objects are at least a single byte wide, to overcome this
limitation the mutex objects contain an embedded coexistent scalar unsigned
integral value that can be used for any other purpose

```c++
auto mutex = sharp::Mutex<std::uint8_t>{};

auto data = mutex.lock_fetch();
cout << *data << endl;
data.unlock();
```

Here the mutex takes up the two most significant bits of the storage allocated
for it and the rest is used up by the coexistent scalar

The API ensures that users only make safe writes to the embedded integer by
following an RAII based access pattern and safely committing to the atomic
integer on destruction.  If the user tries to change the spinlock state, the
program will try and raise an exception or crash with a helpful error message

```c++
auto mutex = sharp::Mutex<std::uint8_t>{};

auto data = mutex.lock_fetch();
*data = 0b11000000;
data.unlock();
```

Here the API will throw saying that the user accidentally overflowed the
storage allocated for the mutex
