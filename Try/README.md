`Try`
-----

Idea taken from facebook's
[`folly::Try`](https://github.com/facebook/folly/blob/master/folly/Try.h)
abstraction with the idea being that an instance of `Try` either contains a
value or an exception but not both.  As such it is internally a wrapper over
`std::variant<Type, std::exception_ptr>`

However is slightly different in semantics.  It resembles a non thread safe
one way only future instance and is logically more like
`std::optional<std::variant<T, std::exception_ptr>>`

```c++
auto value = make_try();
if (value) {
    cout << value.get() << endl;
}
```

Here the `value` will return true when converted to bool if it has a value,
and if it does have a value `.get()` will return that value.

```c++
try {
    auto value = make_try();
    cout << value.get() << endl;
} catch (std::exception& exc) {
    cerr << exc.what() << endl;
}
```

When an instance of `Try` holds an exception, calling `.get()` also throws the
exception.

Since a `Try` can either have a value or not, it does not support convert
assignment operators to convert from either a value or an exception to a Try.

This is used heavily in the `Future` and `Channel` classes as an internal
representation of either a value or an exception
