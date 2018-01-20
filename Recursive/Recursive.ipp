#pragma once

#include <sharp/Recursive/Recursive.hpp>

#include <utility>
#include <type_traits>

namespace sharp {
namespace recursive_detail {

    template <typename Func>
    class RecursiveImpl : public Func {
    public:
        // is the function noexcept invocable
        template <typename F, typename... Args>
        static constexpr const auto is_noexcept_invocable = noexcept(
                std::declval<F>()(std::declval<Args>()...));

        using Self_t = RecursiveImpl;

        template <typename F>
        RecursiveImpl(F&& f) : Func{std::forward<F>(f)} {}

        /**
         * The functions that are going to be invoked by users, these will
         * automatically cast to the lambda type and pasa a self parameter to
         * it, so users are not aware of the abstraction
         */
        template <typename... Args>
        auto operator()(Args&&... args)
            noexcept(is_noexcept_invocable<Func&, Self_t&, Args&&...>)
            -> decltype(std::declval<Func&>()(
                    std::declval<Self_t&>(), std::declval<Args>()...)) {
            return static_cast<Func&>(*this)(
                    *this, std::forward<Args>(args)...);
        }
        template <typename... Args>
        auto operator()(Args&&... args) const
            noexcept(is_noexcept_invocable<const Func&, const Self_t&,
                                           Args&&...>)
            -> decltype(std::declval<const Func&>()(
                            std::declval<const Self_t&>(),
                            std::declval<Args>()...)) {
            return static_cast<const Func&>(*this)(
                    *this, std::forward<Args>(args)...);
        }
    };

} // namespace recursive_detail

template <typename Lambda>
auto recursive(Lambda&& lambda) {
    return recursive_detail::RecursiveImpl<std::decay_t<Lambda>>{
        std::forward<Lambda>(lambda)};
}

} // namespace sharp
