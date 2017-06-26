`Channel`
---------

An implementation of channels as found in the [Go programming
langauge](https://tour.golang.org/concurrency/2)

### Example usage

```c++
#include <numeric>
#include <thread>
#include <vector>
#include <iostream>

#include <sharp/Channel/Channel.hpp>

using std::cout;
using std::endl;

template <typename InputIt>
void sum(InputIt begin, InputIt end, sharp::Channel<int>& c) {
    auto sum = std::accumulate(begin, end, 0);
    c.send(sum);
}

int main() {

    auto s = std::vector<int>{7, 2, 8, -9, 4, 0};

    sharp::Channel<int> c;
    std::thread{[&]() { sum(s.begin(), s.begin() + s.size()/2, c); }}.detach();
    std::thread{[&]() { sum(s.begin() + s.size()/2, s.end(), c); }}.detach();

    auto x = c.read();
    auto y = c.read();

    cout << x << y << x + y << endl;

    return 0;
}
```

### Buffered channels

```c++
#include <iostream>

#include <sharp/Channel/Channel.hpp>

using std::cout;
using std::endl;

int main() {
    sharp::Channel<int> c{2};
    ch.send(1);
    ch.send(2);
    cout << ch.read() << endl;
    cout << ch.read() << endl;

    return 0;
}
```


### Range based channeling

```c++
#include <sharp/Channel/Channel.hpp>
#include <sharp/Range/Range.hpp>

void fibonacci(sharp::Channel<int>& c) {
    auto x = 0, y = 1;

    for (auto i = 0; i < 10; ++i) {

        auto to_send = x, new_y = x + y;
        x = y;
        y = new_y;

        c.send(to_send);
    }

    c.close();
}

int main() {

    sharp::Channel<int> c;
    std::thread{[&]() { fibonacci(c); }}.detach();

    for (auto i : c) {
        cout < i << endl;
    }

    return 0;
}
```


### Compile time channel multiplexing via `select`
The compile time `select` API implementation picks whether you are waiting on
a read or a write based on the signature of the function passed in along with
the channel.

```c++
#include <iostream>

#include <sharp/Channel/Channel.hpp>

using std::cout;

void fibonacci(sharp::Channel<int>& c, sharp::Channel<int>& quit) {
    auto x = 0, y = 1;

    auto should_continue = true;
    while (should_continue) {

        sharp::channel_select(
            std::make_pair(std::ref(c), [&] () -> int {

                auto to_send = x, new_y = x + y;
                x = y;
                y = new_y;

                return to_send;
            }),

            std::make_pair(std::ref(quit), [&](auto) {
                cout << "Quitting" << endl;
                should_continue = false;
            })
        );
    }
}

int main() {
    sharp::Channel<int> c;
    sharp::Channel<int> quit;

    std::thread{[&]() {
        for (auto i = 0; i < 10; ++i) {
            cout << c.read() << endl;
        }
        quit.send(0);
    }}.detach();

    fibonacci(c, quit);
}
```
