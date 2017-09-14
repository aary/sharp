`MoveInto` Self documenting move only parameter passing
----------

This is motivated by the following example related to ambiguity with respect
to rvalue parameter passing.  For example, assume that `foo` and `bar` are two
functions that expect a `Something` instance to be moved into them.  The
functions can look something like this

```c++
void foo(Something&& something) { ... }
void bar(Something something) { ... }
```

Now user code can look like this, with the problems highlighted in the
comments

```c++
auto something = Something{};
foo(std::move(something));

// has something been moved from or is it in the same state it was in before
// the function call?

// oops accidentally copied it into the function
bar(something);
```

Wrapping classes in `MoveInto` makes it clear to the caller that any object
being called will be moved into the function.  Anything else will just not
work

```c++
void foo(MoveInto<Something> something) { ... }
void bar(MoveInto<Something> something) { ... }

int main() {
    auto something = Something{};
    foo(std::move(something));

    // something is in its moved from state

    // won't compile because the function expects a move
    bar(something);
}
```

This can even be used to create variables that should only be moved into from
other variables

```c++
class Wrapper {
public:
    MoveInto<Something> something;
};
```

### Performance

There is no performance cost to using `MoveInto`, it's a simple wrapper that
holds an instance of the type it was templated on.  The only thing to consider
here as opposed to regular parameter passing is that since it is not an
aggregate, prvalues will not be elided into the function parameters, they will
be moved

### Object access

`MoveInto` instances behave like pointers and the internal stored values can
be accessed with the dereference (`*`) operator as well as the arrow (`->`)
operator, similar to `std::optional`
