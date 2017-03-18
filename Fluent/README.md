`Fluent` Named arguments in C++
------------------------------------

You might do something like so to start a server in Python

```Python
start_server(port=80, threads=4, debug=true)
```

With this module you can expose a similar strongly typed interface to 
clients.  The function may be called like so (with any permutation of any
subset of the allowed arguments)

```C++
start_server(Port{80});
start_server(Threads{4});
start_server(Debug{true});

start_server(Port{80}, Threads{4});
start_server(Threads{4}, Port{80});
start_server(Port{80}, Debug{true});
start_server(Debug{true}, Port{80});
start_server(Threads{4}, Debug{true});
start_server(Debug{true}, Threads{4});

start_server(Port{80}, Threads{4}, Debug{true});
start_server(Threads{4}, Port{80}, Debug{true});
start_server(Port{80}, Debug{true}, Threads{4});
start_server(Debug{true}, Port{80, Threads{4}});
start_server(Threads{4}, Debug{true}, Port{80});
start_server(Debug{true}, Threads{4}, Port{80});
```

And to define the function

```C++
template <typename... Args>
void start_server(Args&&... args) {
    auto tuple_args = fluent_get_tuple<Port, Threads, Debug>(args...);

    // the following will print out the values held in default constructed
    // versions of the types unless the user passed in a custom parameter
    cout << std::get<0>(tuple_args) << endl;
    cout << std::get<1>(tuple_args) << endl;
    cout << std::get<2>(tuple_args) << endl;
}
```

### Implementation notes

The module is implemented with template metaprogramming, as you would expect.
The backend for this module comes from `sharp/Traits` which contains an entire
implementation of the STL algorithms but at compile time with type and value
lists
