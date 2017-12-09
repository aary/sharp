`Settings` Event driven flags and settings
----------

```c++
namespace settings = sharp::settings;
namespace {
    auto config = settings::define<int>("config", default_value, "a config");
} // namespace <anonymous>

int main(int args, char** argv) {
    settings::init(args, argv);
    cout << "The value of config is " << *config << endl;
}
```

And you can start the program like this
```
$ ./a.out --config <value>
```

On program start the value of the config will be set to the value specified.
This makes this library an easy way to define flags for your program

Other than being just a flags library, this allows changing the configurations
of the program at runtime
```c++
namespace settings = sharp::settings;
namespace {
    auto config = settings::define<int>("config", default_value, "a config");
} // namespace <anonymous>

int main(int args, char** argv) {
    settings::init(args, argv);

    // use the config
    cout << "The value of config is " << *config << endl;

    // subscribe to changes to the configuration, add a callback to be invoked
    // via the executor ex
    config.via(ex).subscribe([](auto new_value) {
        cout << "The new value of config is " << new_value << endl;
    });

    // publish changes to the config
    settings::publish("config", new_value);
}
```

Which types can be set as settings?  Any type that can be serialized and
deserialized via `std::ostream` and `std::istream`.  This means complex types
like JSON can also be passed through the command line
```c++
namespace settings = sharp::settings;
namespace {
    auto data = settings::define<json>("data", settings::required, "some data");
} // namespace <anonymous>

int main(int argc, char** argv) {
    settings::init(argc, argv);

    cout << data << endl;
    data.via(ex).subscribe([](auto new_value) {
        cout << "Got new data " << new_value << endl;
    });

    settings::publish("data", "{'some key' : 'some value'}");
}
```

```
$ ./a.out --data '{"some key" : "value"}'
```
