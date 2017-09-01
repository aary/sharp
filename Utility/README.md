`Utility` Random helpful things
---------

#### `sharp::match_forward`

A function that very much like std::forward just forward the object passed to
it with the right value category as determined by the type passed to
match_forward.  This can be used to forward another object by considering the
value category of somethin different but related

This is to be used in scenarios where you get a variable of unknown
referenceness and you want to forward another object, maybe related to the
first one with the same reference-ness as the one with unknown reference-ness.
The usage for this function is illustrated below

```c++
template <typename Something>
decltype(auto) forward_another_thing(Something&& something) {
    auto&& another = std::forward<Something>(something).get_another();
    return sharp::match_forward<Something, decltype(another)>(another);
}
```

#### `Crtp`

This class is a utility class that makes making CRTP base classes just a
little more expressive, instead of using the following ugly construct

```c++
static_cast<Derived*>(this)->method()
```

The following much simpler alternative makes things easier

```c++
this->instance().method();
```

#### `move_if_movable`

When classes don't have move constructors defined or when they have a deleted
move constructor `move_of_movable` can be called to be more explicit about
copying and is required in the latter case to initiate a move when the class
has a deleted move constructor

```C++
template <typename T>
void Something::push_to_global(T& object) {
    // move the object into the vector if it's constructible from an rvalue,
    // else copy it
    this->container.push_back(std::move_if_movable(object));
}
```


#### `Type`

A struct meant just to contain a type, this is often a 0 cost abstraction when
code is optimized

### `LessPtr`

`LessPtr` contains a less than comparator for ordering that compares two
pointer-like types but rather than comparing the values of the pointers
themselves, this compares the actual elements pointed to by the pointers.

For example

```
std::set<std::unique_ptr<int>, LessPtr> set_ptrs;
set_ptrs.insert(std::make_unique<int>(1));
set_ptrs.insert(std::make_unique<int>(0));

// output for the following always is "0 1"
for (const auto& ptr : set_ptrs) {
    cout << *ptr << ' ';
}
cout << endl;
```

