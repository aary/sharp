#include <memory>

#include <gtest/gtest.h>
#include <sharp/TransparentList/TransparentList.hpp>

using std::make_unique;
using sharp::TransparentList;
using sharp::Node;

TEST(TransparentList, construct_test) {
    sharp::TransparentList<int>{};
}

// TEST(TransparentList, simple_push_back_test) {
    // auto list = sharp::TransparentList<int>{};
    // auto new_node = new Node<int>{sharp::emplace_construct::tag, 1};
    // list.push_back(new_node);
    // EXPECT_EQ(new_node, &(*list.begin()));
// }

TEST(TransparentList, simple_push_front_test) {
    auto list = sharp::TransparentList<int>{};
    auto node = make_unique<Node<int>>(sharp::emplace_construct::tag, 1);
    list.push_front(node.get());
    EXPECT_EQ(node.get(), &(*list.begin()));
    EXPECT_EQ(1, (*list.begin()).datum);
}

// TEST(TransparentList, double_push_front_test) {

    // auto list = sharp::TransparentList<int>{};

    // auto node_one = make_unique<Node<int>>(sharp::emplace_construct::tag, 1);
    // list.push_front(node_one.get());

    // auto node_two = make_unique<Node<int>>(sharp::emplace_construct::tag, 2);
    // list.push_front(node_two.get());

    // auto node_three = make_unique<Node<int>>(sharp::emplace_construct::tag, 3);
    // list.push_front(node_three.get());

    // EXPECT_EQ(node_three.get(), &(*list.begin()));
    // EXPECT_EQ(3, (*list.begin()).datum);
    // EXPECT_EQ(node_two.get(), &(*(++list.begin())));
    // EXPECT_EQ(2, (*(++list.begin())).datum);
    // EXPECT_EQ(node_two.get(), &(*(++++list.begin())));
    // EXPECT_EQ(2, (*(++++list.begin())).datum);
// }
