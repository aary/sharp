#include <gtest/gtest.h>
#include <sharp/Channel/Channel.hpp>

TEST(Channel, BasicTest) {
    sharp::Channel<int> ch{1};
    ch.send(1);
    EXPECT_EQ(ch.read(), 1);
}

TEST(Channel, TryReadFail) {
    sharp::Channel<int> ch_one;
    EXPECT_FALSE(ch_one.try_read());

    sharp::Channel<int> ch_two{2};
    EXPECT_FALSE(ch_two.try_read());
}
