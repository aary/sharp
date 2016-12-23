LessPtr
-------

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
