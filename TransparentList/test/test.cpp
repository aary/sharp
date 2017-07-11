#include <sharp/TransparentList/TransparentList.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <iostream>
#include <utility>

using std::unique_ptr;
using std::vector;
using std::make_unique;
using sharp::TransparentList;
using sharp::TransparentNode;
using sharp::emplace_construct;

TEST(TransparentList, construct_test) {
    sharp::TransparentList<int>{};
}

TEST(TransparentList, simple_push_back_test) {
    auto list = sharp::TransparentList<int>{};
    auto new_node = make_unique<TransparentNode<int>>(emplace_construct::tag,
            1);
    list.push_back(new_node.get());
    EXPECT_EQ(new_node.get(), *list.begin());
}

TEST(TransparentList, simple_push_front_test) {
    auto list = sharp::TransparentList<int>{};
    auto node = make_unique<TransparentNode<int>>(emplace_construct::tag, 1);
    list.push_front(node.get());
    EXPECT_EQ(node.get(), *list.begin());
    EXPECT_EQ(1, (*list.begin())->datum);
}

TEST(TransparentList, double_push_front_test) {

    auto list = sharp::TransparentList<int>{};

    auto node_one = make_unique<TransparentNode<int>>(emplace_construct::tag,
            1);
    list.push_front(node_one.get());

    auto node_two = make_unique<TransparentNode<int>>(emplace_construct::tag,
            2);
    list.push_front(node_two.get());

    auto node_three = make_unique<TransparentNode<int>>(emplace_construct::tag,
            3);
    list.push_front(node_three.get());

    EXPECT_EQ(node_three.get(), *list.begin());
    EXPECT_EQ(3, (*list.begin())->datum);
    EXPECT_EQ(node_two.get(), *(++list.begin()));
    EXPECT_EQ(2, (*(++list.begin()))->datum);
    EXPECT_EQ(node_one.get(), *(++++list.begin()));
    EXPECT_EQ(1, (*(++++list.begin()))->datum);
}

TEST(TransparentList, double_push_back_test) {

    auto list = sharp::TransparentList<int>{};

    auto node_one = make_unique<TransparentNode<int>>(emplace_construct::tag,1);
    list.push_back(node_one.get());

    auto node_two = make_unique<TransparentNode<int>>(emplace_construct::tag,2);
    list.push_back(node_two.get());

    auto node_three = make_unique<TransparentNode<int>>(emplace_construct::tag,
            3);
    list.push_back(node_three.get());

    EXPECT_EQ(node_one.get(), *list.begin());
    EXPECT_EQ(1, (*list.begin())->datum);
    EXPECT_EQ(node_two.get(), *(++list.begin()));
    EXPECT_EQ(2, (*(++list.begin()))->datum);
    EXPECT_EQ(node_three.get(), *(++++list.begin()));
    EXPECT_EQ(3, (*(++++list.begin()))->datum);
}

TEST(TransparentList, range_test) {

    auto list = sharp::TransparentList<int>{};
    auto vec = vector<unique_ptr<TransparentNode<int>>>{};
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 1));
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 2));
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 3));
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 4));

    // insert into the list in order
    for (const auto& node : vec) {
        list.push_back(node.get());
    }

    // assert that the ranges are equal
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [](const auto& node_ptr_lhs, const auto& node_ptr_rhs) {
        return node_ptr_lhs->datum == node_ptr_rhs->datum;
    }));
}

TEST(TransparentList, range_test_and_push_back_front_test) {

    auto list = sharp::TransparentList<int>{};
    auto vec = vector<unique_ptr<TransparentNode<int>>>{};
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 1));
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 2));
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 3));
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 4));

    // insert into the list in order
    for (const auto& node : vec) {
        list.push_back(node.get());
    }

    // assert that the ranges are equal
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [](const auto& node_ptr_lhs, const auto& node_ptr_rhs) {
        return node_ptr_lhs->datum == node_ptr_rhs->datum;
    }));

    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 5));
    list.push_back(vec.back().get());

    // assert that the ranges are equal
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [](const auto& node_ptr_lhs, const auto& node_ptr_rhs) {
        return node_ptr_lhs->datum == node_ptr_rhs->datum;
    }));
}

TEST(TransparentList, test_erase) {
    auto list = sharp::TransparentList<int>{};
    auto vec = vector<unique_ptr<TransparentNode<int>>>{};
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 1));
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 2));
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 3));

    for (const auto& node : vec) {
        list.push_back(node.get());
    }

    auto iter = std::find_if(list.begin(), list.end(), [&](auto node) {
        return node->datum == 2;
    });
    EXPECT_TRUE(iter != list.end());
    list.erase(iter);

    vec.erase(vec.begin() + 1);
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [](const auto& node_one, const auto& node_two) {
        return node_one->datum == node_two->datum;
    }));

    list.erase(list.begin());
    vec.erase(vec.begin());
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [](const auto& node_one, const auto& node_two) {
        return node_one->datum == node_two->datum;
    }));

    vec.insert(vec.begin(),
            make_unique<TransparentNode<int>>(emplace_construct::tag, 2));
    iter = list.insert(list.begin(), vec.front().get());
    list.erase(iter);
    vec.erase(vec.begin());
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [](const auto& node_one, const auto& node_two) {
        return node_one->datum == node_two->datum;
    }));
}

TEST(TransparentList, test_increment_decrement_iterators) {
    auto list = sharp::TransparentList<int>{};
    auto vec = vector<unique_ptr<TransparentNode<int>>>{};
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 1));
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 2));
    vec.push_back(make_unique<TransparentNode<int>>(emplace_construct::tag, 3));

    for (const auto& node : vec) {
        list.push_back(node.get());
    }

    auto iter = list.begin();
    EXPECT_EQ((*iter)->datum, 1);
    ++iter;
    EXPECT_EQ((*iter)->datum, 2);
    --iter;
    EXPECT_EQ((*iter)->datum, 1);
    ++iter;
    EXPECT_EQ((*iter)->datum, 2);
    ++iter;
    EXPECT_EQ((*iter)->datum, 3);
    ++iter;
    EXPECT_EQ(iter, list.end());
}

TEST(TransparentList, test_insert) {
    auto list = sharp::TransparentList<int>{};
    auto one = make_unique<TransparentNode<int>>(emplace_construct::tag, 1);
    auto two = make_unique<TransparentNode<int>>(emplace_construct::tag, 2);
    list.insert(list.begin(), one.get());
    list.insert(list.begin(), two.get());

    auto vec = std::vector<unique_ptr<TransparentNode<int>>>{};
    vec.push_back(std::move(two));
    vec.push_back(std::move(one));

    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [&](const auto& node_one, const auto& node_two) {
        return node_one->datum == node_two->datum;
    }));
}
