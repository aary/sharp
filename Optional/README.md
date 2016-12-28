`Optional`
----------

This module contains an optional module that extends the regular functionality
provided by the standard library optional to include one more mode of
operation - compact optionals.  Compact optionals can be useful when there is
a strict memory requirement, if and when there is a strict memory requirement,
compact optionals use much less memory than the regular optional type.  In
particular they do not contain a boolean to denote whether a type exists or
not and do not contain the padding associated with that boolean, which can at
times be siginificant, for example

```
struct Something {
    bool boolean;
    std::int32_t four_byte_integer;
};
```

The memory footprint of a `Something` object is significantly bloated because
of the extra boolean contained within it, the size of the struct needs to be
aligned to a multiple of the largest type and as such the size degrades to 8
bytes when only 5 bytes are needed.  Compact optionals solve this problem by
occupying only the amount of space needed to store the actual type, for
example if an optional type was being built around an `std::int32_t` object
then the compact optional would also occupy only 4 bytes.

The caveat with compact optionals however is that there needs to be a special
value reserved with the type that represents a "null" value, in other words a
sentinal value needs to exist for the underlying data distribution.  For
example if it is known that the `std::int32_t` cannot contain any negative
value, then the sentinal can be a `-1`, or in many cases a suitable value for
the sentinal can simply be a `0`.
