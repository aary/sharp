#include <sharp/TransparentList/TransparentList.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <iostream>
#include <utility>
#include <list>
#include <deque>

using std::unique_ptr;
using std::vector;
using std::make_unique;
using std::cout;
using std::endl;

namespace sharp {

TEST(TransparentList, construct_test) {
    sharp::TransparentList<int>{};
}

TEST(TransparentList, simple_push_back_test) {
    auto list = sharp::TransparentList<int>{};
    auto new_node = make_unique<TransparentNode<int>>(std::in_place, 1);
    list.push_back(new_node.get());
    EXPECT_EQ(new_node.get(), *list.begin());
}

TEST(TransparentList, simple_push_front_test) {
    auto list = sharp::TransparentList<int>{};
    auto node = make_unique<TransparentNode<int>>(std::in_place, 1);
    list.push_front(node.get());
    EXPECT_EQ(node.get(), *list.begin());
    EXPECT_EQ(1, (*list.begin())->datum);
}

TEST(TransparentList, double_push_front_test) {

    auto list = sharp::TransparentList<int>{};

    auto node_one = make_unique<TransparentNode<int>>(std::in_place, 1);
    list.push_front(node_one.get());

    auto node_two = make_unique<TransparentNode<int>>(std::in_place, 2);
    list.push_front(node_two.get());

    auto node_three = make_unique<TransparentNode<int>>(std::in_place, 3);
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

    auto node_one = make_unique<TransparentNode<int>>(std::in_place ,1);
    list.push_back(node_one.get());

    auto node_two = make_unique<TransparentNode<int>>(std::in_place, 2);
    list.push_back(node_two.get());

    auto node_three = make_unique<TransparentNode<int>>(std::in_place, 3);
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
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 1));
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 2));
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 3));
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 4));

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
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 1));
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 2));
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 3));
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 4));

    // insert into the list in order
    for (const auto& node : vec) {
        list.push_back(node.get());
    }

    // assert that the ranges are equal
    EXPECT_TRUE(std::equal(vec.begin(), vec.end(), list.begin(), list.end(),
                [](const auto& node_ptr_lhs, const auto& node_ptr_rhs) {
        return node_ptr_lhs->datum == node_ptr_rhs->datum;
    }));

    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 5));
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
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 1));
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 2));
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 3));

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
            make_unique<TransparentNode<int>>(std::in_place, 2));
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
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 1));
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 2));
    vec.push_back(make_unique<TransparentNode<int>>(std::in_place, 3));

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
    auto one = make_unique<TransparentNode<int>>(std::in_place, 1);
    auto two = make_unique<TransparentNode<int>>(std::in_place, 2);
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

TEST(TransparentList, test_splice) {
    auto one = std::deque<std::shared_ptr<TransparentNode<int>>>{};
    one.push_back(std::make_shared<TransparentNode<int>>(std::in_place, 1));
    one.push_back(std::make_shared<TransparentNode<int>>(std::in_place, 2));
    one.push_back(std::make_shared<TransparentNode<int>>(std::in_place, 3));

    auto two = std::deque<std::shared_ptr<TransparentNode<int>>>{};
    two.push_back(std::make_shared<TransparentNode<int>>(std::in_place, 4));
    two.push_back(std::make_shared<TransparentNode<int>>(std::in_place, 5));

    // test insertion in the front
    [=]() mutable {
        auto list_one = sharp::TransparentList<int>{};
        for (auto& ptr : one) {
            list_one.push_back(ptr.get());
        }

        auto list_two = sharp::TransparentList<int>{};
        for (auto& ptr : two) {
            list_two.push_back(ptr.get());
        }

        list_one.splice(list_one.begin(), list_two);
        std::copy(two.rbegin(), two.rend(), std::front_inserter(one));

        EXPECT_TRUE(std::equal(list_one.begin(), list_one.end(), one.begin(),
                               one.end(), [](auto ptr, auto s_ptr) {
            return ptr == s_ptr.get();
        }));
    }();

    // test insertion at the end
    [=]() mutable {
        auto list_one = sharp::TransparentList<int>{};
        for (auto& ptr : one) {
            list_one.push_back(ptr.get());
        }

        auto list_two = sharp::TransparentList<int>{};
        for (auto& ptr : two) {
            list_two.push_back(ptr.get());
        }

        list_one.splice(list_one.end(), list_two);
        std::copy(two.begin(), two.end(), std::back_inserter(one));

        EXPECT_TRUE(std::equal(list_one.begin(), list_one.end(), one.begin(),
                               one.end(), [](auto ptr, auto s_ptr) {
            return ptr == s_ptr.get();
        }));
    }();

    // test insertion in the middle
    [=]() mutable {
        auto list_one = sharp::TransparentList<int>{};
        for (auto& ptr : one) {
            list_one.push_back(ptr.get());
        }

        auto list_two = sharp::TransparentList<int>{};
        for (auto& ptr : two) {
            list_two.push_back(ptr.get());
        }

        list_one.splice(std::next(list_one.begin()), list_two);
        for (auto i = two.rbegin(); i != two.rend(); ++i) {
            one.insert(std::next(one.begin()), *i);
        }

        EXPECT_TRUE(std::equal(list_one.begin(), list_one.end(), one.begin(),
                               one.end(), [](auto ptr, auto s_ptr) {
            return ptr == s_ptr.get();
        }));
    }();
}

} // namespace sharp
