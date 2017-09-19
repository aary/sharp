#include <iostream>
#include <typeinfo>
#include <typeindex>

using std::cout;
using std::endl;

int main() {
    auto t_index_one = std::type_index{typeid(int)};
    auto t_index_two = std::type_index{typeid(int)};
    // auto t_index_one = 1;
    // auto t_index_two = 1;
    volatile auto sum = 0;

    for (auto i = 0; i < 1e9; ++i) {
        if (t_index_one == t_index_two) {
            ++sum;
        }
    }
}
