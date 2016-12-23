OrderedContainer
----------------

`OrderedContainer` contains an interface that is compiled on top of the
existing interfaces for containers that allows them to be used in an ordered
fashion, in the best way possible.  So for example if the goal is to make a
linked list that is sorted, then you can do the following

```
OrderedContainer<std::list<int>> ordered_list;

// insert values into the list
ordered_list.insert(1);
ordered_list.insert(0);
ordered_list.insert(2);

auto iter = ordered_list.find(1)
if (iter != ordered_list.end()) {
    cerr << "Could not find a 1 in the container" << endl;
}
```

The library also offers users a way to fetch the underlying container for its
specific functionality like so

```
auto& inner_list = ordered_list.get();
static_assert(std::is_same<
    std::list<int>,
    std::decay_t<decltype(inner_list)>>::value,
    "OrderedContainer is incorrect, get() returns the wrong type");
```
