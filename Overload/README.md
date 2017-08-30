`Overload` Overload anything together
----------

```c++
void foo(char) {}

auto overload = sharp::make_overload([&](double) {}, [&](std::string) {}, foo);
overload(1.2);
overload("something");
overload('a');
```

This can be very useful and can be used to make code readable and maintainable
with complex double dispatch and visitor patterns

```c++
void handle_int(int) {}
void handle_double(double) {}

auto variant = std::variant<int, double, std::string>{};
std::visit(sharp::make_overload(
        [&some_state](std::string&) {},
        handle_int,
        handle_double),
    variant);
```
