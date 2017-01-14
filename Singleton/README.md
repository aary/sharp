`Singleton`
-----------

Inspired by folly::Singleton this module contains generalized singleton class
that allows classes to offer a singleton interface to users.  The class
ensures that singletons that are depenedant on each other are destroyed and
created in such a way that all references are pointing to singleton instances
that are valid.  This implies that the singletons are automatically created
and destroyed in a manner that would serialize their dependencies.

```
#include <sharp/Singleton/Singleton.hpp>

class TheSingleton {
public:
    /**
     * declare the singleton class a friend so it can create an instance of
     * this class
     */
    friend class Singleton<TheSingleton>;

    TheSingleton(const TheSingleton&) = delete;
    TheSingleton(TheSingleton&&) = delete;
    TheSingleton& operator=(const TheSingleton&) = delete;
    TheSingleton& operator=(TheSingleton&&) = delete;

private:
    TheSingleton();
};

int main() {
    auto& singleton = sharp::Singleton<TheSingleton>::get();
    // use singleton
}
```

The C++ standard specifies that destruction of static objects will proceed in
the reverse order of destruction, but there is no good way to serialize and
generalize destruction of such objects at end time if there is no relation to
the order of construction,  this API offers such a possibility, by creating an
instance of a proxy object like so
`static Singleton<Type>::register_destruct_dependence<SOne, STwo> dep;` You
can register a destruction dependence for the singletons conveniently
