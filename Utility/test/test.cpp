#include <sharp/Utility/Utility.hpp>

#include <gtest/gtest.h>

#include <type_traits>
#include <set>
#include <memory>
#include <cstdint>
#include <utility>
#include <cstdlib>
#include <ctime>

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

    template <typename One>
    class OneMonad : public sharp::VariantMonad<One> {
    public:
        ~OneMonad() {
            auto&& ref = this->template cast<One>();
            static_cast<void>(ref);
        }
        template <typename T>
        int test() {
            auto&& ref = this->template cast<T>();
            static_cast<void>(ref);
            return 1;
        }
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

TEST(Utility, BasicAsConstTest) {
    auto integer = 1;
    EXPECT_TRUE((std::is_same<decltype(sharp::as_const(integer)), const int&>
                ::value));
}

TEST(Utility, BasicDecayCopyTest) {
    auto integer = 1;
    auto&& copied_or_what = sharp::decay_copy(integer);
    EXPECT_NE(&integer, &copied_or_what);
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

TEST(Utility, LessPtrTest) {
    std::set<std::unique_ptr<int>, sharp::LessPtr> integer_ptrs;

    // make two pointers and disorder them based on where they actually point
    // to on the heap, make the smaller one come after the larger one
    auto one = std::make_unique<int>(1);
    auto two = std::make_unique<int>(2);

    // disorder
    if (reinterpret_cast<std::uintptr_t>(one.get())
            < reinterpret_cast<std::uintptr_t>(two.get())) {
        std::swap(*one, *two);
    }

    // insert and assert that the first one comes before the second even
    // though it comes after in memory
    integer_ptrs.insert(std::move(one));
    integer_ptrs.insert(std::move(two));
    EXPECT_LT(**integer_ptrs.begin(), **(--integer_ptrs.end()));

    // assert that the finding is working correctly after inserting noise
    std::srand(std::time(nullptr));
    for (auto i = 0; i < 10; ++i) {

        // find a number that is not equal to 1 or 2
        auto to_insert = int{};
        do {
            to_insert = std::rand();
        } while (to_insert == 1 || to_insert == 2);

        // insert it into the set
        integer_ptrs.insert(std::make_unique<int>(to_insert));
    }

    static constexpr const auto ONE = 1;
    static constexpr const auto TWO = 2;
    EXPECT_NE(integer_ptrs.find(&ONE), integer_ptrs.end());
    EXPECT_NE(integer_ptrs.find(&TWO), integer_ptrs.end());
}

TEST(Utility, VariantMonadBasicTest) {
    auto int_double_monad = sharp::VariantMonad<int, double>{};
    new (&int_double_monad.cast<int>()) int{2};
    EXPECT_EQ(int_double_monad.cast<int>(), 2);
}

TEST(Utility, VariantMonadTestNoTemplateKeywordRequired) {
    auto int_monad = OneMonad<int>{};
    EXPECT_EQ(int_monad.test<int>(), 1);
}
