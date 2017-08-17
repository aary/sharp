`for_each` Iterate through anything
----------

This library provides an API for looping and iterating through any type of
range at all.  This does not have to be a runtime range like a `std::vector`
but can also be a "range" that is defined at compile time like a `std::tuple`

The library has high differential complexity as you use more features but the
inherent simple use is really simple.  Even simpler than the existing
`std::for_each` algorithm

```c++
// With the standard std::for_each
std::for_each(range.begin(), range.end(), [](auto element) {
    cout << element << endl;
});

// With sharp::for_each
sharp::for_each(range, [](auto element) {
    cout << element << endl;
});
```

Iterate with indices

```c++
sharp::for_each(std::make_tuple(1, 2), [](auto element, auto index) {
    cout << index << " : " << element << endl;
});
```

Iterate through a list and peek forwards and backwards with iterators

```c++
auto range = std::list<int>{1, 2, 3};
sharp::for_each(range, [](auto element, auto index, auto iterator) {
    if (should_remove(element, index)) {
        range.erase(iterator);
    }
});
```

### `sharp::fetch`

A convenience wrapper is provided to help make uniform the syntax for indexing
and accessing elements within a range.

```c++
auto one = std::make_tuple(1, 2, 3);
auto two = std::tuple<int, int, int>{};

sharp::for_each(one, [](auto element, auto index) {
    sharp::fetch(two, index) = element * 2;
});
```

Similarly for other "runtime" ranges

```c++
auto one = std::vector<int>{1, 2, 3};
auto two = std::vector<int>(3);

sharp::for_each(one, [](auto element, auto index) {
    sharp::fetch(two, index) = element * 2;
});
```

The loop code can remain the same no matter what.

Take a look inside the headers `ForEach.hpp` and `ForEach.ipp` for more
documentation on the exact workings of the algorithm and utility

### Performance benchmarks

Further performance benchmarks show that the library performs just as well as
a native loop and in some cases can even be faster.
