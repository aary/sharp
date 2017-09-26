#include <sharp/Try/Try.hpp>
#include <sharp/Utility/Utility.hpp>

#include <gtest/gtest.h>

namespace {

    class DestroySignal {
    public:
        DestroySignal(bool& signal_in) : signal{signal_in} {}
        ~DestroySignal() {
            this->signal = true;
        }

        bool& signal;
    };

    class TestCopy {
    public:
        static int count_copies;
        TestCopy() = default;
        TestCopy(const TestCopy&) {
            ++count_copies;
        }
        TestCopy(TestCopy&&) = delete;
    };
    int TestCopy::count_copies = 0;

    class TestMove {
    public:
        static int count_moves;
        TestMove() = default;
        TestMove(const TestMove&) = delete;
        TestMove(TestMove&&) {
            ++count_moves;
        }
    };
    int TestMove::count_moves = 0;

    class TestCopyMove {
    public:
        static int count_copies;
        static int count_moves;
        TestCopyMove() = default;
        TestCopyMove(TestCopyMove&&) {
            ++count_moves;
        }
        TestCopyMove(const TestCopyMove&) {
            ++count_copies;
        }
    };
    int TestCopyMove::count_copies = 0;
    int TestCopyMove::count_moves = 0;

    class TestFromInt {
    public:
        static int count_moves;
        static int count_copies;
        TestFromInt() = delete;
        TestFromInt(const TestFromInt&) = delete;
        TestFromInt(TestFromInt&&) = delete;
        explicit TestFromInt(int&&) {
            ++count_moves;
        }
        explicit TestFromInt(const int&) {
            ++count_copies;
        }
    };
    int TestFromInt::count_copies = 0;
    int TestFromInt::count_moves = 0;

} // namespace anonymous

class TryTest : public ::testing::Test {
public:
    void SetUp() override {
        TestCopy::count_copies = 0;
        TestMove::count_moves = 0;
        TestFromInt::count_copies = 0;
        TestFromInt::count_moves = 0;
    }
};

TEST_F(TryTest, DefaultConstruct) {
    auto t = sharp::Try<int>{};
    EXPECT_FALSE(t.has_value());
    EXPECT_FALSE(t.has_exception());
    EXPECT_FALSE(t.valid());
    EXPECT_FALSE(t.is_ready());
    EXPECT_FALSE(t);
}

TEST_F(TryTest, Destructor) {
    {
        auto signal = false;
        {
            auto t = sharp::Try<DestroySignal>{std::in_place, signal};
        }
        EXPECT_TRUE(signal);
    }
    {
        auto signal = false;
        {
            auto t = sharp::Try<DestroySignal>{DestroySignal{signal}};
        }
        EXPECT_TRUE(signal);
    }
}

TEST_F(TryTest, TestCopyConstructor) {
    sharp::Try<TestCopy> one{std::in_place};
    auto two = one;
    EXPECT_EQ(TestCopy::count_copies, 1);
}

TEST_F(TryTest, TestMoveConstructor) {
    auto one = sharp::Try<TestMove>{std::in_place};
    auto two = std::move(one);
    EXPECT_EQ(TestMove::count_moves, 1);
}

TEST_F(TryTest, TestOtherConstructor) {
    auto one = sharp::Try<int>{1};
    sharp::Try<TestFromInt> a{one};
    sharp::Try<TestFromInt> b{std::move(one)};
    static_cast<void>(a);
    static_cast<void>(b);
    static_cast<void>(one);

    EXPECT_EQ(TestFromInt::count_copies, 1);
    EXPECT_EQ(TestFromInt::count_moves, 1);
}

TEST_F(TryTest, TestElementConstructor) {
    TestCopy one{};
    sharp::Try<TestCopy>{one};
    EXPECT_EQ(TestCopy::count_copies, 1);
}

TEST_F(TryTest, TestInPlaceConstructor) {
    auto exc = std::logic_error{"some error"};
    auto exc_ptr = std::make_exception_ptr(exc);
    auto one = sharp::Try<int>{exc_ptr};
    EXPECT_THROW(one.value(), std::logic_error);
    EXPECT_THROW(one.get(), std::logic_error);
}

TEST_F(TryTest, NullPtrConstruct) {
    auto t = sharp::Try<int>{nullptr};
    EXPECT_FALSE(t.has_value());
    EXPECT_FALSE(t.has_exception());
    EXPECT_FALSE(t.valid());
    EXPECT_FALSE(t.is_ready());
    EXPECT_FALSE(t);
}

TEST_F(TryTest, ValueTest) {
    using sharp::as_const;
    auto t = sharp::Try<TestCopyMove>{std::in_place};
    EXPECT_TRUE((std::is_same<decltype(t.value()), TestCopyMove&>::value));
    EXPECT_TRUE((std::is_same<decltype(as_const(t).value()),
                              const TestCopyMove&>::value));
    EXPECT_TRUE((std::is_same<decltype(std::move(t).value()),
                              TestCopyMove&&>::value));
    EXPECT_TRUE((std::is_same<decltype(std::move(as_const(t)).value()),
                              const TestCopyMove&&>::value));
}

TEST_F(TryTest, DereferenceTest) {
    using sharp::as_const;
    auto t = sharp::Try<TestCopyMove>{std::in_place};
    EXPECT_TRUE((std::is_same<decltype(*t), TestCopyMove&>::value));
    EXPECT_TRUE((std::is_same<decltype(*as_const(t)),
                              const TestCopyMove&>::value));
    EXPECT_TRUE((std::is_same<decltype(*std::move(t)), TestCopyMove&&>::value));
    EXPECT_TRUE((std::is_same<decltype(*std::move(as_const(t))),
                              const TestCopyMove&&>::value));
}
