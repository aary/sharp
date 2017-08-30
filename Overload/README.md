`Overload` Overload anything
----------

```c++
char foo(char ch) {
    return ch;
}

int main() {
    auto overload = sharp::make_overload(
        [&](double d) { return d; },
        [&](std::string str) { return str; },
        foo);

    assert(overload(1.2) == 1.2
    assert(overload("something") == "something");
    assert(overload('a') == 'a');
}
```

This can be very useful and can be used to make code readable and maintainable
with complex double dispatch and visitor patterns

```c++
void handle_int(int) {}
void handle_double(double) {}

int main() {
    // ...

    auto variant = std::variant<int, double, std::string>{};
    std::visit(sharp::make_overload(
            [&some_state](std::string&) {},
            handle_int,
            handle_double),
        variant);
}
```
