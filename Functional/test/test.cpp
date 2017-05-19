#include <memory>

#include <gtest/gtest.h>
#include <sharp/Functional/Functional.hpp>

TEST(Functional, BasicFunctional) {
    // move only lambda that would otherwise not be compatible with
    // std::function
    auto int_uptr = std::make_unique<int>(2);
    auto f = sharp::Function<int()>{[int_uptr = std::move(int_uptr)]() {
        return (*int_uptr) * 2;
    }};
    EXPECT_EQ(f(), 4);
}
