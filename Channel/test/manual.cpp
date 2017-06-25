#include <iostream>
#include <type_traits>
#include <deque>
#include <exception>

using std::cout;
using std::endl;

int main() {
    auto d = std::deque<std::aligned_union<0, int, std::exception_ptr>>{};

    d.emplace_back();
    new (&d.back()) int{2};
    for (auto& element : d) {
        cout << *(reinterpret_cast<int*>(&element)) << endl;
    }

    d.emplace_back();
    new (&d.back()) int{3};
    for (auto& element : d) {
        cout << *(reinterpret_cast<int*>(&element)) << endl;
    }
}
