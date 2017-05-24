`Range`
-------

A Pythonic range class for C++

```C++
for (auto integer : range(0, 10)) {
    cout << integer << endl;
}
```

```C++
auto v = std::vector<int>{1, 2, 3};
for (auto integer : range(v.begin(), v.end())) {
    cout << integer << endl;
}
