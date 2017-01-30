`Channel`
---------

A type safe channel like the similar one in Go.  The module also provides a
similar construct for buffered channels but in a way that does not require a
limit to the buffer size, which would seem limiting.

### Example usage

```c++
#include <numeric>
#include <thread>
#include <vector>
#include <iostream>

#include <sharp/Channel/Channel.hpp>

using std::cout;
using std::endl;

template <typename InputIterator>
void sum(InputIterator begin, InputIterator end, sharp::Channel<int>& c) {
    auto sum = std::accumulate(begin, end, 0);
    c.send(sum);
}

int main() {

    std::vector<int> s{7, 2, 8, -9, 4, 0};

    auto c = sharp::Channel<int>{};
    std::thread{[]() { sum(s.begin(), s.begin() + s.size()/2, c); }}.detach();
    std::thread{[]() { sum(s.begin() + s.size()/2, s.end(), c); }}.detach();

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
    auto ch = sharp::Channel<int>{2};
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

void fibonacci(int n, sharp::Channel<int>& c) {
    auto x = 0;
    auto y = 1;

    for (auto i : sharp::range(0, n)) {
        c.send(i);

        auto new_y = x + y;
        x = y;
        y = new_y;
    }

    c.close();
}

int main() {

    auto c = sharp::Channel<int>{};
    fibonacci(10, c);

    for (auto i : c) {
        cout < i << endl;
    }

    return 0;
}
```


### Channel Multiplexing via `select`
The `select` API implementation picks whether you are waiting on a read or a
write based on the signature of the function passed in along with the channel.

```c++
#include <iostream>

#include <sharp/Channel/Channel.hpp>

using std::cout;

void fibonacci(sharp::Channel<int>& c, sharp::Channel<int>& quit) {
    auto x = 0;
    auto y = 1;

    auto should_continue = true;
    while (should_continue) {
        sharp::Channel::select(
            {c, [&] () -> int {

                auto to_send = x;
                auto new_y = x + y;
                x = y;
                y = new_y;

                return to_send;
            }},

            {quit, [&](auto) {
                cout << "quit" << endl;
                should_continue = false;
            }}
        );
    }
}
```
