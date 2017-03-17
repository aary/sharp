`TypeTraits.hpp`
----------------

This module contains simple template metaprogramming classes (and traits) that
occasionally help in writing template code.

Among other things here, there is a full implementation of the <algorithm>
header and its functionality as it appeared pre-C++17 in the form of template
metaprogramming and manipulations with type lists (as opposed to the
traditional runtime value range concept).

For example say you were creating a container type that was not allowed to
contain any references.  You could do something like this

```
template <typename... Types>
struct CannotContainReferences {
    static_assert(sharp::AnyOf_v<std::is_reference, Types...>, "No refs!");
};
```
