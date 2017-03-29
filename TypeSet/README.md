`TypeSet`
---------

`TypeSet` is a container that encapsulates objects of distinct types and
provides O(0) access to objects of a given type, i.e. there is no computation
to get from a type to an object, the computation happens at compile time.
Accessing an object of the given type from the container is instantaneous,
like accessing a local variable of that type.
