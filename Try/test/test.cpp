#include <sharp/Try/Try.hpp>

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
} // namespace anonymous

TEST(Try, DefaultConstruct) {
    auto t = sharp::Try<int>{};
    EXPECT_FALSE(t.has_value());
    EXPECT_FALSE(t.has_exception());
    EXPECT_FALSE(t.valid());
    EXPECT_FALSE(t.is_ready());
    EXPECT_FALSE(t);
}

TEST(Try, NullPtrConstruct) {
    auto t = sharp::Try<int>{nullptr};
    EXPECT_FALSE(t.has_value());
    EXPECT_FALSE(t.has_exception());
    EXPECT_FALSE(t.valid());
    EXPECT_FALSE(t.is_ready());
    EXPECT_FALSE(t);
}

TEST(Try, Destructor) {
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
