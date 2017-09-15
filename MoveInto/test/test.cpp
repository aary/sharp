#include <sharp/Traits/Traits.hpp>
#include <sharp/MoveInto/MoveInto.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <cstddef>
#include <cstdint>

namespace {

int* foo(sharp::MoveInto<std::unique_ptr<int>> ptr) {
    return ptr->get();
}

template <typename T, typename = sharp::void_t<>>
class CheckCopyDisallowed : public std::integral_constant<bool, true> {};
template <typename T>
class CheckCopyDisallowed<T, sharp::void_t<
    decltype(sharp::MoveInto<T>{std::declval<T&>()})>>
        : public std::integral_constant<bool, false> {};

} // namespace <anonymous>

TEST(MoveInto, Basic) {
    auto u_ptr = std::make_unique<int>(1);
    sharp::MoveInto<std::unique_ptr<int>> ptr{std::move(u_ptr)};
    EXPECT_EQ(**ptr, 1);
}

TEST(MoveInto, BasicCallFunction) {
    auto u_ptr = std::make_unique<int>(1);
    auto ptr = u_ptr.get();
    auto result = foo(std::move(u_ptr));
    EXPECT_EQ(ptr, result);
}

TEST(MoveInto, CheckCopyNotAllowed) {
    EXPECT_TRUE((CheckCopyDisallowed<std::shared_ptr<int>>::value));
    EXPECT_TRUE((CheckCopyDisallowed<const std::shared_ptr<int>>::value));
}
