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
auto range = std::make_tuple(1, 2);
sharp::for_each(range, [](auto element, auto index) {
    cout << index << " : " << element << endl;
});
```

You can also break from a loop preemptively

```c++
auto range = std::make_tuple(1, 2);
sharp::for_each(range, [](auto element, auto index) {
    if (index >= 2) {
        return sharp::loop_break;;
    }

    cout << index << " : " << element << endl;
    return sharp::loop_continue;
});
```

Iterate through a list and peek forwards and backwards with iterators

```c++
auto range = std::list<int>{1, 2, 3};
sharp::for_each(range, [&](auto element, auto index, auto iterator) {
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
auto three = std::vector<int>{0, 0, 0};
auto four = std::unordered_map<int, int>{};

sharp::for_each(one, [](auto element, auto index) {
    sharp::fetch(two, index) = element * 2;
    sharp::fetch(three, index) = element * 2;
    sharp::fetch(four, index) = element * 2;
});
```

The loop code can remain the same no matter what.

Take a look inside the headers `ForEach.hpp` and `ForEach.ipp` for more
documentation on the exact workings of the algorithm and utility

### Avoiding common looping pitfalls

```c++
for (const auto element : make_optional(vec).value()) {
    cout << element << endl;
}
```

Spot the bug?

The expression on the right in the range based for loop is an xvalue, and an
xvalue's lifetime is not guaranteed to be extended, and in this case it is not
extended.  As a result you will be looping over a container that has already
been destroyed.

If however you were to use `sharp::for_each`, this problem would be alleviated

```c++
sharp::for_each(make_optional(vec).value(), [](auto element) {
    cout << element << endl;
});
```

Here the lifetime of the object bound to the function parameter of
`sharp::for_each` will be extended until the function has finished executing.
Therefore all looping code that you write will execute on a safe range that
will be destroyed after the loop

### Performance benchmarks

Tuple-like loops are unrolled manually and cause an O(1) template
instantiation overhead.  So build times are not greatly affected

Further performance benchmarks show that the library performs just as well as
a native loop and in some cases can even be faster.
