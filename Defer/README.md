`Defer`
-------

A module implementing `defer` similar to the in-built language construct in Go
(https://blog.golang.org/defer-panic-and-recover), with the same semantics
similar usage.  There is no reason to include a panic and a recover
implementation because C++ offers a different, and more efficient alternative
(try and catch blocks)

The following is an example of how you would go about using defer to force
close a database connection in an RAII manner

```
void something() {
    auto database_connection_ptr = connect_db();

    // mark the database to be closed before exit
    auto deferred = defer([&]() {
        database_connection_ptr->close();
    });

    // use database
}
```

Similarly defer calls can be chained and each deferable function call is
pushed onto a stack and when stack is collapsed the functions are ran in the
reverse order of registration

```
void top_level() {
    auto database_connection_ptr = connect_db();

    auto deferred = defer([&]() {
        cout << "Closing the database connection" << endl;
        database_connection_ptr->close();
    });

    if (database_connection_ptr.is_under_heavy_load()) {
        database_connection_ptr->spawn_threads_for_low_latency();
        auto deferred = defer([&]() {
            database_connection_ptr->join_threads_for_low_latency();
            cout << "Destroying threads before closing" << endl;
        });
    }

    // use database
}
```

And this will output the following when the second if block executes

```
Destroying threads before closing
Closing the database connection
```
