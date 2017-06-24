#include <gtest/gtest.h>
#include <sharp/Channel/Channel.hpp>

TEST(Channel, BasicTest) {
    sharp::Channel<int> ch{1};
    ch.send(1);
    EXPECT_EQ(ch.read(), 1);
}
