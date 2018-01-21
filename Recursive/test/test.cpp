#include <sharp/Recursive/Recursive.hpp>

#include <gtest/gtest.h>

#include <tuple>
#include <iostream>

using std::cout;
using std::endl;

namespace sharp {

namespace {
    class RecursiveTestCopyMoveConstruct {
    public:
        RecursiveTestCopyMoveConstruct() = default;
        RecursiveTestCopyMoveConstruct(const RecursiveTestCopyMoveConstruct&) {
            ++global_copies;
        }
        RecursiveTestCopyMoveConstruct(RecursiveTestCopyMoveConstruct&&) {
            ++global_moves;
        }

        static int global_copies;
        static int global_moves;
    };
    int RecursiveTestCopyMoveConstruct::global_copies = 0;
    int RecursiveTestCopyMoveConstruct::global_moves = 0;
} // namespace

TEST(RecursiveTest, Basic) {
    auto sum = sharp::recursive([](auto& self, auto start, auto end, auto sum) {
        if (start == end) {
            return sum;
        }

        return self(start + 1, end, sum + start);
    });

    EXPECT_EQ(sum(0, 5, 0), 10);
}

TEST(RecursiveTest, TestCapture) {
    auto sum = 0;
    auto sum_function = sharp::recursive([&](auto& self, auto start, auto end) {
        if (start == end) {
            return;
        }
        sum += start;
        self(start + 1, end);
    });

    sum_function(0, 5);
    EXPECT_EQ(sum, 10);
}

TEST(RecursiveTest, TestCopyAndMoveOutside) {
    RecursiveTestCopyMoveConstruct::global_copies = 0;
    RecursiveTestCopyMoveConstruct::global_moves = 0;
    auto instance = RecursiveTestCopyMoveConstruct{};

    auto func = sharp::recursive([=](auto&) -> void {
        std::ignore = instance;
    });
    auto another = func;

    EXPECT_EQ(RecursiveTestCopyMoveConstruct::global_copies, 2);
}

TEST(RecursiveTest, TestCopyInsideClosure) {
    RecursiveTestCopyMoveConstruct::global_copies = 0;
    RecursiveTestCopyMoveConstruct::global_moves = 0;
    auto instance = RecursiveTestCopyMoveConstruct{};
    auto integer = 1;

    sharp::recursive([&, instance](auto self) {
        std::ignore = instance;
        if (++integer == 3) {
            return;
        }
        self();
    })();

    EXPECT_EQ(integer, 3);

    // passing by value to itself should not have caused a move, but only
    // copies, as the copy is made at the point of invocation which has no
    // knowledge about the lifetime of the parameter itself
    //
    // one copy is made in the initial capture when the closure is created,
    // then the closure is moved into the wrapper in sharp::recursive(), and
    // then the two copies come from copy passing the wrapped closure into
    // itself
    EXPECT_EQ(RecursiveTestCopyMoveConstruct::global_copies, 3);
    EXPECT_EQ(RecursiveTestCopyMoveConstruct::global_moves, 1);
}

} // namespace sharp
