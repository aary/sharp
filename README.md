`sharp`
-------

This repository contains code for some C++ libraries that I have implemented
in my free time.

## Some highlights

### `sharp::Channel`
An implementation of channels similar to the one found in Go, with all the
features and more (like exceptions)
```c++
sharp::select(
    sharp::case(channel, [](auto ele) {
        cout << "Read value from channel " << ele << endl;
    }),
    sharp::case(channel, []() -> int {
        return value;
    })
);

channel.send(1);
```

### `sharp::for_each`
Iterate through anything, with one consistent interface
```c++
auto tup = std::make_tuple(1, 2, 3, 4);
sharp::for_each(tup, [](auto ele, auto index) {
    if (ele == 2 || index == 3) {
        return sharp::loop_break;
    }
    return sharp::loop_continue;
});

// iterate through a range of lambdas
auto functions = std::make_tuple([]() { return 1; }, []() { return 2; });
sharp::for_each(functions, [](auto func) {
    cout << func() << endl;
});
```

### `sharp::Concurrent`
A monitor or simple lock abstraction that is safe and pleasant to use
```c++
auto concurrent_vec = sharp::Concurrent<std::vector>{};

// thread 1
auto vec = concurrent_vec.lock();
lock.wait([](auto& vec) {
    return vec.size() >= 2;
});
cout << vec->size() << endl;

// thread 2
concurrent_vec.synchronized([](auto& vec) {
    vec.push_back(1);
    vec.push_back(2);
});
```

### Pythonic stuff
Just some cool stuff
```c++
for (auto [ele, index] : sharp::enumerate(vector)) {
    cout << ele << " " << index << endl;
}
```
```c++
for (auto ele : sharp::range(0, 12)) {
    cout << ele << endl;
}
```
```c++
for (auto [a, b] : sharp::zip(range_one, range_two)) {
    cout << a << " " << b << endl;
}
```

### `sharp::Settings`
A library for providing settings and configurations to your programs, either
as flags or as real time configs.  With an interface similar to
[`gflags`](https://gflags.github.io/gflags/), but typed.  This serves similar
purposes but with a strictly better, more extendible interface
```c++
namespace settings = sharp::settings;
namespace {
    auto config = settings::define<int>("config", default_value, "a config");
} // namespace <anonymous>

int main(int args, char** argv) {
    settings::init(args, argv);
    cout << "The value of config is " << *config << endl;
}
```

And then pass the flag to the program like so
```
$ ./a.out --config <value>
```

### `sharp::overload`
Overload anything, anywhere with no overhead at all.  All at compile time
(surprisingly hard to get right)
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

### `sharp::Invoke`
Function calls which allow users to used named arguments but in a type safe
manner at compile time.
```c++
auto value = sharp::get(args::Host{"https://some_api.com"}, args::Timeout{1s});
cout << value << endl;
```

With more supported features like optional arguments, one in many arguments,
etc.

### `sharp::Future`
An implementation of futures that fixes some common issues with futures like
unnecessary locking on ready fetching, with non-opaque threading models
because of non-sticky executors etc.  Microbenchmarks also show that these are
faster (and more flexible) than boost futures
```c++
auto one = async_io();
auto two = async_io();
return sharp::when_all(one, two).via(cpu).then([](auto [one, two]) {
    return one.get() * two.get();
});
```

### `sharp::Recursive`
Write recursive closures at 0 runtime cost
```c++
auto sum = sharp::recursive([](auto& self, auto start, auto end, auto sum) {
    if (start == end) {
        return sum;
    }

    return self(start + 1, end, sum + start);
});
assert(sum(0, 5, 0), 10);
```

### `sharp::Try`
Represent either a value or an exception in a classy way
```c++
auto [value, error] = &get_value_or_error();
if (value) {
    cout << *value << endl;
} else if (error) {
    cout << *error << endl;
}
```

### Metaprogramming

`sharp/Traits` contains a complete implementation of the `<algorithm>` header
with sequences of compile time ranges, for example

```c++
using Sorted = sharp::Sort_t<LessThan, Types...>;
```

And more...

## Building

The libraries in this project support building with
[Buck](https://buckbuild.com).  To install buck from the latest stable commit
on master, run the following

```
brew install facebook/fb/buck
```

This will install buck with homebrew using [Facebook's homebrew
tap](https://github.com/facebook/homebrew-fb).

This repository is arranged in the form of a buck module.  The recommended
way of installing this library into your project is by using  git submodules.

```
git submodule add https://github.com/aary/sharp
git submodule update --init --recursive
```

I would recommend cloning the example project I have
[here](https://github.com/aary/sharp-example) to see how a project would use
`sharp` and build with buck.

To use this with buck simply add a `[repositories]` section in your top level
`.buckconfig` as follows

```
[repositories]
    sharp = ./sharp
```

Then build with buck by adding a dependency of the following form
`sharp//:sharp`.  For example the `deps` section in the `BUCK` file for your
project can look like so

```
deps = [
    "sharp//:sharp",
],
```

and the corresponding include statement in your C++ file would be

```
#include <sharp/Concurrent/Concurrent.hpp>
```

If any bugs are found please open an issue on
[Github](https://github.com/aary/sharp).

## Running unit tests

[Google test](https://github.com/google/googletest) needs to be present in the
sharp folder to run the unit tests.  It has been included as a git submodule
in this repository.  Run the following to install `gtest` within this project
```
git submodule update --init --recursive
```

Once `gtest` has been installed `cd` into the `sharp` folder and run all unit
tests with `buck`
```
buck test ...
```

To run the unit tests individually for a module run `buck test
<module_name>/...` from the `sharp` folder.  For example to run unit tests for
the `Concurrent` module run `buck test Concurrent/...`
