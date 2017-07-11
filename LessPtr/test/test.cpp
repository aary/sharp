#include <sharp/LessPtr/LessPtr.hpp>

#include <gtest/gtest.h>

#include <set>
#include <memory>
#include <cstdint>
#include <utility>
#include <cstdlib>
#include <ctime>

using namespace std;

TEST(Less, BasicTest) {
    std::set<std::unique_ptr<int>, sharp::LessPtr> integer_ptrs;

    // make two pointers and disorder them based on where they actually point
    // to on the heap, make the smaller one come after the larger one
    auto one = std::make_unique<int>(1);
    auto two = std::make_unique<int>(2);

    // disorder
    if (reinterpret_cast<std::uintptr_t>(one.get())
            < reinterpret_cast<std::uintptr_t>(two.get())) {
        std::swap(*one, *two);
    }

    // insert and assert that the first one comes before the second even
    // though it comes after in memory
    integer_ptrs.insert(std::move(one));
    integer_ptrs.insert(std::move(two));
    EXPECT_LT(**integer_ptrs.begin(), **(--integer_ptrs.end()));

    // assert that the finding is working correctly after inserting noise
    std::srand(std::time(nullptr));
    for (auto i = 0; i < 10; ++i) {

        // find a number that is not equal to 1 or 2
        auto to_insert = int{};
        do {
            to_insert = std::rand();
        } while (to_insert == 1 || to_insert == 2);

        // insert it into the set
        integer_ptrs.insert(std::make_unique<int>(to_insert));
    }

    static constexpr const auto ONE = 1;
    static constexpr const auto TWO = 2;
    EXPECT_NE(integer_ptrs.find(&ONE), integer_ptrs.end());
    EXPECT_NE(integer_ptrs.find(&TWO), integer_ptrs.end());
}
