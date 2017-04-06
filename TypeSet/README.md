`TypeSet`
---------

`TypeSet` is a container that encapsulates objects of distinct types and
provides O(0) access to objects of a given type, i.e.  there is no runtime
computation to get from a type to an object, all computation happens at
compile time.  Accessing an object of a type from the container is
instantaneous, like accessing a local variable of that type.

## Using `TypeSet` to imitate named arguments

The motivation behind building this was to allow users to call functions in a
manner that conveyed its behavior clearly.  Similar to named argument passing
in many languages, `TypeSet` can be used to imitate named arguments to
functions, for example

```C++
/**
 * @function start_server
 * @param Port a port instance can be passed that will determine the port
 * @param Debug whether or not the server should run in debug mode
 *
 * Starts a server with the passed arguments
 */
template <typename... Args>
void start_server(Args&&... args) {

    // collect the arguments into a type set, those not passed will be default
    // constructed
    auto args = sharp::collect_args<Port, Debug>(std::forward<Args>(args)...);

    // set the port to run on
    this->set_port(sharp::get<Port>(args));

    // make debug configurations
    if (sharp::get<Debug>(args).is_set()) { ... }
}

// start server on port 8000 and in debug mode
start_server(Port{8000}, Debug{true});
start_server(Debug{true}, Port{8000});

// start server on the default port and in debug mode
start_server(Debug{true});

// start server on port 8000 and in the default debug mode
start_server(Port{8000});
```

## Optimizations

The container is optimized to be space efficient.  The layout of the objects
internally is done in such a manner so as to reduce unnecesary memory
consumption for alignment and padding.  For example the following two structs
differ in their memory consumption

```C++
struct A {
    double d;
    int i;
    char c;
};

struct B {
    char c;
    double d;
    int i;
};
```

The first takes 16 bytes on my system and the second takes 24 bytes.  If you
make a type set with a double, int and char it will automatically be arranged
so that the object is as space efficient as possible.

Further all operations done on the container transfer the value category of
the container to its internal data members.  For example if you have a
temporary type set, then accessing its data members will return them as
movable entities.
