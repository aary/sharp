`Invoke` Super charged function calls
--------

```c++
void func(int a, double d, const std::string& str) {
    cout << a << " " << d << " " << str << endl;
}

int main() {
    sharp::invoke(func, 1, 1.2, "something");
    sharp::invoke(func, 1.2, 1, "something");
    sharp::invoke(func, "something", 1, 1.2);
    sharp::invoke(func, "something", 1.2, 1);
    sharp::invoke(func, 1.2, "something", 1);
    sharp::invoke(func, 1, "something", 1.2);
}
```

This utility can be used to match a function call to any of its arguments in
any order, the user does not have to know the order in which functions are
expected to be delivered.  They only need to know which arguments to pass

#### Named arguments

```c++
STRONG_TYPEDEF(int, Port);
STRONG_TYPEDEF(std::string, Host);

auto make_server(Port port, Host host) {
    return Server{port, host};
}

int main() {
    auto server = sharp::invoke(make_server, Host{"localhost"}, Port{80});
    server.run();
}
```

This lets you create a list of arguments that are distinguished by strong
typedefs.  Strong typedefs while not supported by the language are mimicked by
the macro `STRONG_TYPEDEF`, which is similar to boost's `BOOST_STRONG_TYPEDEF`
but supports more modern C++ features

#### Optional arguments

```c++
STRONG_TYPEDEF(int, Port);
STRONG_TYPEDEF(std::string, Host);

auto make_server(std::optional<Port> port, std::optional<Host> host) {
    return Server{port.value_or(80), host.value_or("localhost")};
}

int main() {
    auto server = sharp::invoke(make_server, Port{8080});
    server.run();
}
```

`sharp::invoke` has built in support for optional types, so if an argument is
not supplied, the optional type will be used to provide a default value.

#### Variant arguments

```c++
STRONG_TYPEDEF(int, Port);
STRONG_TYPEDEF(std::string, Host);

auto make_server(std::optional<Port> port, std::variant<int, double> options) {}

int main() {
    // valid
    sharp::invoke(make_server, Port{80}, 2);
    sharp::invoke(make_server, Port{80}, 3.2);

    // invalid, will not compile
    sharp::invoke(make_server, 2.1, 2);
    sharp::invoke(make_server, Port{80}, 2, 2.1);
}
```

`std::variant` can be stressed to emphasize that only one of many arguments is
required, and the library will `static_assert` to ensure that's the case
