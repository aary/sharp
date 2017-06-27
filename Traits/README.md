`Traits`
----------------

This module contains simple template metaprogramming classes (and traits) that
occasionally help in writing template code.

Among other things here, there is a full implementation of the `<algorithm>`
header and its functionality as it appeared pre-C++17 in the form of template
metaprogramming and manipulations with type lists (as opposed to the
traditional runtime value range concept).

Say you were designing a type set.  You would want the types to be arranged in
increasing order of their sizes to as to take the least extra alignment needed
via padding, for example the following struct is more space optimized than the
latter

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

You can sort the type list.  Make a comparator as usual and then pass the list
and the comparator to the sort function

```C++
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

```C++
template <typename... Types>
struct CannotContainReferences {
    static_assert(!sharp::AnyOf_v<std::is_reference, Types...>, "No refs!");
};
```

## Algorithms

#### `std::all_of`
```C++
static_assert(sharp::AllOf_v<std::is_pointer, int*, double*>);
```

#### `std::any_of`

```C++
static_assert(sharp::AnyOf_v<std::is_reference, int, double, int&>);
```

#### `std::none_of`

```C++
static_assert(sharp::NoneOf_v<std::is_const, int, double, char>);
```

### `std::for_each`

```C++
sharp::ForEach<std::tuple<int, double>>{}([](auto identity) {
    cout << typeid(typename decltype(identity)::type).name() << endl;
}
```

#### `std::count_if`

```C++
static_assert(sharp::Countif_v<std::is_reference, int, double, int&> == 1);
```

#### `std::mismatch`

```C++
static_assert(std::is_same<sharp::Mismatch_t<std::tuple<int, double, char>,
                                             std::tuple<int, float, char>>,
                           std::pair<std::tuple<double, char>,
                                     std::tuple<float, char>>>::value);
```

#### `std::equal`

```C++
static_assert(sharp::Equal_v<std::tuple<int, double>,
                             std::tuple<int, double, char>>);
```

#### `std::find_if`

```C++
static_assert(std::is_same<sharp::FindIf_t<std::is_reference, int, char&, int>,
                           std::tuple<char&, int>>::value);
```

#### `std::find`

```C++
static_assert(std::is_same<sharp::Find_t<int, std::tuple<double, int, char>>,
                           std::tuple<int, char>>::value);
```

#### `std::find_if_not`

```C++
static_assert(std::is_same<sharp::FindIfNot_t<std::is_reference, int&, int>,
                           std::tuple<int>>::value);
```

#### `std::find_first_of`

```C++
static_assert(std::is_same<sharp::FindFirstOf_t<std::tuple<int, char, bool>,
                                                std::tuple<double, char>>,
                           std::tuple<char, bool>>::value);
```

#### `std::adjacent_find`

```C++
static_assert(std::is_same<sharp::AdjacentFind_t<int, double, double, int>,
                           std::tuple<double, double, int>>::value);
```

#### `std::search`

```C++
static_assert(std::is_same<sharp::Search_t<std::tuple<int, double, char, bool>,
                                           std::tuple<double, char>>,
                           std::tuple<double, char, bool>>::value);
```

#### `std::search_n`

```C++
static_assert(std::is_same<sharp::SearchN_t<int, 2, int, bool, int, int, char>,
                           std::tuple<int, int, char>>::value);
```

#### `std::transform_if`

```C++
static_assert(std::is_same<
    sharp::TransformIf_t<std::is_volatile,
                         std::remove_volatile, volatile int, const int>,
    std::tuple<int, const int>>::value);
```

#### `std::transform`

```C++
static_assert(std::is_same<
    sharp::Transform_t<std::remove_volatile, volatile int, const int>,
    std::tuple<int, const int>>::value);
```

#### `std::remove_if`

```C++
static_assert(std::is_same<sharp::RemoveIf_t<std::is_reference, char, int&>,
                           std::tuple<char>>::value);
```

#### `std::reverse`

```C++
static_assert(std::is_same<sharp::Reverse_t<int, char>,
                           std::tuple<char, int>>::value);
```

#### `std::unique`

```C++
static_assert(std::is_same<sharp::Unique_t<int, double, int>,
                           std::tuple<int, double>>::value);
```

#### `std::sort`

```C++
static_assert(std::is_same<Sort_t<SizeCompare, uint32_t, uint16_t, uint8_t>,
                           std::tuple<uint8_t, uint16_t, uint32_t>>::value);
```

#### `std::max`

```C++
static_assert(std::is_same<MaxType_t<SizeCompare, uint8_t, uint16_t>,
                                     uint16_t>::value);
static_assert(MaxValue_v<1, 4, 0> == 4);
```

#### `std::min`

```C++
static_assert(std::is_same<MinType_t<SizeCompare, uint8_t, uint16_t>,
                                     uint8_t>::value);
static_assert(MinValue_v<1, 4, 0> == 0);
```


## Function traits

#### `sharp::ReturnType`

```C++
int foo(char, double, float);
static_assert(std::is_same<ReturnType_t<decltype(foo)>, int>::value);
```

#### `sharp::Arguments`

```C++
int foo(char, double, float);
static_assert(std::is_same<Arguments_t<decltype(foo)>,
                           std::tuple<char, double, float>>::value);
```

#### `sharp::IsCallable`

```C++
int foo(char, double, float);
static_assert(IsCallable_v<decltype(foo)>);
```

## Functional traits

#### `sharp::Bind`

```C++
static_assert(NoneOf_v<Bind<std::is_same, int>::template type,
                       std::tuple<double, char, float>>);
```

#### `sharp::Negate`

```C++
static_assert(
    NoneOf_v<Negate<Bind<std::is_same, int>::template type>::template type
    std::tuple<double, char, float>>);
```


## Utility traits

#### `sharp::IsInstantiationOf`

```C++
static_assert(sharp::IsInstantiationOf_v<std::vector<int>, std::vector>);
```

#### `sharp::Concatenate_t`

```C++
static_assert(std::is_same<Concatenate_t<std::tuple<int>, std::tuple<double>>,
                           std::tuple<int, double>>::value);
```

#### `sharp::ConcatenateN`

```C++
static_assert(std::is_same<ConcatenateN_t<double, 2>,
                           std::tuple<double, double>>::value);
```

#### `sharp::PopFront`

```C++
static_assert(std::is_same<PopFront_t<std::tuple<int, double>>,
                           std::tuple<double>>::value);
```

#### `sharp::Erase`

```C++
static_assert(std::is_same<Erase_t<1, std::tuple<int, char, double>>,
                           std::tuple<int, double>>::value);
```

#### `sharp::for_each`

```C++
auto tup = make_tuple(1, "1.0");
for_each(tup, [](auto& member) {
    cout << member << endl;
}
```

#### `sharp::MatchReference`

```C++
static_assert(std::is_same<MatchReference_t<int&&, double>, double&&>::value);
```

#### `sharp::match_forward`

```C++
template <typename Something, typename Else>
void foo(Something&&, Else&& else) {
    bar(sharp::match_forward<Something, Else>(else));
}
```

#### `sharp::Crtp`

```C++
template <typename Derived>
class SomeCrtpMixin : public sharp::Crtp<Derived> {
public:
    std::string serialize() const {
        auto ss = std::stringstream{};
        ss << this->this_instance().data;
        return ss.str();
    }
};
```

#### `sharp::move_if_movable`

```C++
template <typename T>
void Something::push_to_global(T& object) {
    // move the object into the vector if it's constructible from an rvalue,
    // else copy it
    this->container.push_back(std::move_if_movable(object));
}
```
