`Traits.hpp`
----------------

This module contains simple template metaprogramming classes (and traits) that
occasionally help in writing template code.

Among other things here, there is a full implementation of the `<algorithm>`
header and its functionality as it appeared pre-C++17 in the form of template
metaprogramming and manipulations with type lists (as opposed to the
traditional runtime value range concept).

Say you were designing a type set (i.e. a tuple that can only consist of
distinct types, for example std::tuple<int, double, char> is a valid type set
whereas std::tuple<int, double, int> is not valid).  You would want the types
to be arranged in increasing order of their sizes to as to take the least
extra alignment needed via padding, for example the following struct is more
space optimized than the latter

```
struct ContainerOptimized {
    uint8_t one;
    uint32_t two;
};

struct ContainerBad {
    uint32_t one;
    uint8_t two;
};
```

You can sort the type list.  Make a comparator as usual and then pass the list
and the comparator to the sort function

```
template <typename... Types>
struct TypeSet {
private:
    template <typename One, typename Two>
    struct LessThan {
        static constexpr const bool value = sizeof(One) < sizeof(Two);
    };

public:
    Sort_t<LessThan, Types...> space_optimized_tuple_member;
};
```

Or if you were creating a container type that was not allowed to contain any
references.  You could do something like this

```
template <typename... Types>
struct CannotContainReferences {
    static_assert(!sharp::AnyOf_v<std::is_reference, Types...>, "No refs!");
};
```


#### `std::all_of`
```
static_assert(sharp::AllOf_v<std::is_pointer, int*, double*>);
```

#### `std::any_of`

```
static_assert(sharp::AnyOf_v<std::is_reference, int, double, int&>);
```

#### `std::none_of`

```
static_assert(sharp::NoneOf_v<std::is_const, int, double, char>);
```

### `std::for_each`

```
sharp::ForEach<std::tuple<int, double>>{}([](auto identity) {
    cout << typeid(typename decltype(identity)::type).name() << endl;
}
```

#### `std::count_if`

```
static_assert(sharp::Countif_v<std::is_reference, int, double, int&> == 1);
```

#### `std::mismatch`

```
static_assert(std::is_same<sharp::Mismatch_t<std::tuple<int, double, char>,
                                             std::tuple<int, float, char>>,
                           std::pair<std::tuple<double, char>,
                                     std::tuple<float, char>>>::value);
```

#### `std::equal`

```
static_assert(sharp::Equal_v<std::tuple<int, double>,
                             std::tuple<int, double, char>>);
```

#### `std::find_if`

```
static_assert(std::is_same<sharp::FindIf_t<std::is_reference, int, char&, int>,
                           std::tuple<char&, int>>::value);
```

#### `std::find_if_not`

```
static_assert(std::is_same<sharp::FindIfNot_t<std::is_reference, int&, int>,
                           std::tuple<int>>::value);
```

#### `std::find_first_of`

```
static_assert(std::is_same<sharp::FindFirstOf_t<std::tuple<int, char, bool>,
                                                std::tuple<double, char>>,
                           std::tuple<char, bool>>::value);
```

#### `std::adjacent_find`

```
static_assert(std::is_same<sharp::AdjacentFind_t<int, double, double, int>,
                           std::tuple<double, double, int>>::value);
```

#### `std::search`

```
static_assert(std::is_same<sharp::Search_t<std::tuple<int, double, char, bool>,
                                           std::tuple<double, char>>,
                           std::tuple<double, char, bool>>::value);
```

#### `std::search_n`

```
static_assert(std::is_same<sharp::SearchN_t<int, 2, int, bool, int, int, char>,
                           std::tuple<int, int, char>>::value);
```

#### `std::transform_if`

```
static_assert(std::is_same<
    sharp::TransformIf_t<std::is_volatile,
                         std::remove_volatile, volatile int, const int>,
    std::tuple<int, const int>>::value);
```

#### `std::transform`

```
static_assert(std::is_same<
    sharp::Transform_t<std::remove_volatile, volatile int, const int>,
    std::tuple<int, const int>>::value);
```

#### `std::remove_if`

```
static_assert(std::is_same<sharp::RemoveIf_t<std::is_reference, char, int&>,
                           std::tuple<char>>::value);
```

#### `std::reverse`

```
static_assert(std::is_same<sharp::Reverse_t<int, char>,
                           std::tuple<char, int>>::value);
```

#### `std::unique`

```
static_assert(std::is_same<sharp::Unique_t<int, double, int>,
                           std::tuple<int, double>>::value);
```

#### `std::sort`

```
static_assert(std::is_same<Sort_t<SizeCompare, uint32_t, uint16_t, uint8_t>,
                           std::tuple<uint8_t, uint16_t, uint32_t>>::value);
```
