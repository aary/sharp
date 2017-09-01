#include <sharp/Utility/Utility.hpp>

#include <gtest/gtest.h>

#include <type_traits>

using namespace sharp;

namespace {

    template <typename Derived>
    class Base : public Crtp<Base<Derived>> {
    public:
        auto foo() {
            return this->instance().a;
        }
    };

    class Derived : public Base<Derived> {
    public:
        int a{1};
    };

    class NotMoveConstructible {
    public:
        NotMoveConstructible() = default;
        NotMoveConstructible(const NotMoveConstructible&) = default;
        NotMoveConstructible(NotMoveConstructible&&) = delete;
    };

    class MoveConstructible {
    public:
        MoveConstructible() = default;
        MoveConstructible(const MoveConstructible&) = default;
        MoveConstructible(MoveConstructible&&) = default;
    };

} // namespace <anonymous>

TEST(Utility, CrtpBasic) {
    auto instance = Derived{};
    EXPECT_EQ(reinterpret_cast<uintptr_t>(&instance.instance()),
            reinterpret_cast<uintptr_t>(&instance));
    EXPECT_EQ(instance.foo(), 1);
}

TEST(Utility, MoveIfMovableBasic) {
    EXPECT_TRUE((std::is_same<
            decltype(sharp::move_if_movable(std::declval<MoveConstructible>())),
            MoveConstructible&&>::value));
    EXPECT_TRUE((std::is_same<
            decltype(sharp::move_if_movable(
                    std::declval<NotMoveConstructible>())),
            const NotMoveConstructible&>::value));
}

TEST(Traits, match_forward_lvalue_to_lvalue) {

    int to_cast;

    // try casting when both of the things are lvalues
    auto&& one = sharp::match_forward<int&, int&>(to_cast);
    EXPECT_TRUE((std::is_lvalue_reference<decltype(one)>::value));

    // try casting when the first thing is an rvalue and the second is an
    // lvalue, rvalues come in two forms when using forwarding references
    //
    //  template <typename Something>
    //  void something(Something&&)
    //
    // Here if the thing passed to the function something() is an rvalue the
    // type of it will be deduced to std::remove_reference<Something> so when
    // you forward with a call like this
    //
    //  sharp::match_forward<Something, Else>(else);
    //
    // The type that you will get will be std::remove_reference<Something>
    // with no reference qualifiers.
    //
    // The other possibility is when the user uses auto in a deduced context
    //
    //  [](auto&& something) { ... }
    //
    // Here the only thing they can do to forward something else with the
    // value type of something is to use decltype(something) and that
    // evaluates to std::remove_reference_t<decltype(something)>&&
    //
    // Therefore match forward should be able to match the rvalue-ness of the
    // first type in both the cases above, when there are two reference signs
    // and when there are not
    auto&& two = sharp::match_forward<int&&, int&>(to_cast);
    EXPECT_TRUE((std::is_rvalue_reference<decltype(two)>::value));
    auto&& three = sharp::match_forward<int, int&>(to_cast);
    EXPECT_TRUE((std::is_rvalue_reference<decltype(three)>::value));

    // now try casting when the second thing is an rvalue, represented both by
    // no reference qualifier above as well as two reference qualifiers,
    // the reasoning for which is explained above
    //
    // The first case when the first thing is an lvalue should not compile
    // becasue forwarding an rvalue as an lvalue should be ill formed as it is
    // prone to errors
    auto&& four = sharp::match_forward<int, int&&>(int{});
    EXPECT_TRUE((std::is_rvalue_reference<decltype(four)>::value));
    auto&& five = sharp::match_forward<int&&, int&&>(int{});
    EXPECT_TRUE((std::is_rvalue_reference<decltype(five)>::value));

    auto&& six = sharp::match_forward<int, int>(int{});
    EXPECT_TRUE((std::is_rvalue_reference<decltype(six)>::value));
    auto&& seven = sharp::match_forward<int&&, int>(int{});
    EXPECT_TRUE((std::is_rvalue_reference<decltype(seven)>::value));
}
