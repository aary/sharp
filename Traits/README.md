`TypeTraits.hpp`
----------------

This module contains simple template metaprogramming classes (and traits) that
occasionally help in writing template code.

Among other things here, there is a full implementation of the <algorithm>
header and its functionality as it appeared pre-C++17 in the form of template
metaprogramming and manipulations with type lists (as opposed to the
traditional runtime value range concept).

For example say that you were creating a
