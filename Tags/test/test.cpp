#include <sharp/Tags/Tags.hpp>

#include <gtest/gtest.h>

#include <type_traits>
#include <cassert>

/*
 * Enum and overloaded functions to test correct tag dispatch
 */
enum class TagType {NORMAL, TYPE, INTEGRAL};
template <typename Tag>
TagType which_tag(typename Tag::tag_t) {
    return TagType::NORMAL;
}
template <typename Tag, typename Type>
TagType which_tag(typename Tag::template tag_type_t<Type>) {
    return TagType::TYPE;
}
template <typename Tag, int INTEGRAL>
TagType which_tag(typename Tag::template tag_integral_t<INTEGRAL>) {
    return TagType::INTEGRAL;
}

TEST(TagsTest, TestGeneralizedTag) {
    /*
     * A test tag that is generated from a generalized tag
     */
    class test_tag : public sharp::GeneralizedTag<test_tag> {};

    EXPECT_EQ(which_tag<test_tag>(test_tag::tag), TagType::NORMAL);
    EXPECT_EQ(which_tag<test_tag>(test_tag::tag_type<int>), TagType::TYPE);
    EXPECT_EQ(which_tag<test_tag>(test_tag::tag_integral<1>),
            TagType::INTEGRAL);
}

/**
 * Runs a test similar to the above for the tag type passed in as a template
 * parameter
 */
template <typename Tag>
static void test_which_tag_for_tag();

TEST(TagsTest, TestEachDefinedTag) {

    // run the test for each tag type
    test_which_tag_for_tag<sharp::initializer_list_construct>();
    test_which_tag_for_tag<sharp::emplace_construct>();
    test_which_tag_for_tag<sharp::delegate_constructor>();
    test_which_tag_for_tag<sharp::implementation>();
}

template <typename Tag>
static void test_which_tag_for_tag() {
    EXPECT_EQ(which_tag<Tag>(Tag::tag), TagType::NORMAL);
    EXPECT_EQ(which_tag<Tag>(Tag::template tag_type<int>), TagType::TYPE);
    EXPECT_EQ(which_tag<Tag>(Tag::template tag_integral<1>), TagType::INTEGRAL);
}
