`Overload` Overload anything
----------

```c++
char foo(char ch) {
    return ch;
}

int main() {
    auto overloaded = sharp::overload(
        [&](double d) { return d; },
        [&](std::string str) { return str; },
        foo);

    assert(overloaded(1.2) == 1.2
    assert(overloaded("something") == "something");
    assert(overloaded('a') == 'a');
}
```

This can be useful and can be used to make code readable and maintainable with
complex double dispatch and visitor patterns

```c++
void handle_int(int) {}
void handle_double(double) {}

int main() {
    // ...

    auto variant = std::variant<int, double, std::string>{};
    std::visit(
        sharp::overload([&](std::string&) {}, handle_int, handle_double),
        variant);
}
```

### Performance

This utility has no runtime performance hit.  The overload dispatching is done
at compile time and the empty base optimizations are applied everywhere.  The
size of overloaded functor instances is also just a single byte when no
captures are done.  The memory cost for functions however is a single pointer
for each function.

However, there is a non trivial O(n) template instantiation overhead, where n
are the number of functions or functors passed to the utility.
