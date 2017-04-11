`Defer`
-------

A module implementing `defer` similar to the in-built language construct in Go
(https://blog.golang.org/defer-panic-and-recover), with the same semantics
similar usage.  There is no reason to include a panic and a recover
implementation because C++ offers a different, and more efficient alternative
(try and catch blocks)

The following is an example of how you would go about using defer to force
close a database connection in an RAII manner

```C++
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

```C++
void top_level() {
    auto database_connection_ptr = connect_db();

    // close the database connection before this scope finishes
    auto deferred_one = defer([&]() {
        database_connection_ptr->close();
    });

    // wait for any threads that the database has running to finish before
    // exiting
    //
    // the difference between defer_guard and defer is analagous to the
    // difference between unique_lock and lock_guard, defer_guard does not
    // branch when executing the destructor, in case the user has reset() the
    // deferred state
    auto deferred_two = defer_guard([&]() {
        if (database_connection_ptr->has_running_threads()) {
            database_connection_ptr->join_threads();
        }
    });

    // use database
}
```
