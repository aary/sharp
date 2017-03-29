`TypeSet`
---------

`TypeSet` is a container that encapsulates objects of distinct types and
provides O(0) access to objects of a given type, i.e.  there is no runtime
computation to get from a type to an object, all computation happens at
compile time.  Accessing an object of a type from the container is
instantaneous, like accessing a local variable of that type.

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
