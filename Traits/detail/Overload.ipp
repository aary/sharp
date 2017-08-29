#pragma once

#include "Overload.hpp"

#include <utility>
#include <type_traits>

template <typename...> struct WhichType;

namespace sharp {
namespace overload_detail {

    /**
     * The base overload template that is an incomplete type, this should
     * never be instantiated.  And even if it is, it should cause a hard error
     */
    template <typename... Funcs>
    class Overload;

    /**
     * The general case, in this case the Overload class imports the
     * operator() method of the current functor class, and then also imports
     * the operator() recursively on the rest of the type list
     *
     * As a result in the first case of the recursion, or in the most derived
     * class.  The most derived class will have imported all the important
     * operator() methods into the current scope
     */
    template <typename Func, typename... Funcs>
    class Overload<Func, Funcs...>
            : public Func,
              public Overload<Funcs...> {
    public:
        template <typename F, typename... Fs>
        explicit Overload(F&& f, Fs&&... fs)
            : Func{std::forward<F>(f)},
            Overload<Funcs...>{std::forward<Fs>(fs)...} {}

        /**
         * Import operator() of the current functor and then recursve
         */
        using Func::operator();
        using Overload<Funcs...>::operator();
    };

    /**
     * Base case, just provide a constructor and import the operator() of the
     * current functor type
     */
    template <typename Func>
    class Overload<Func> : public Func {
    public:
        template <typename F>
        explicit Overload(F&& f)
            : Func{std::forward<F>(f)} {}

        using Func::operator();
    };

    /**
     * Convert lambdas to lambdas function pointers to lambdas
     *
     * This is needed to get the code to work for some reason.  Still
     * investingating why its needed
     */
    template <typename Func>
    decltype(auto) to_functor(Func&& func) {
        return std::forward<Func>(func);
    }
    template <typename ReturnType, typename... Args>
    auto to_functor(ReturnType (*func) (Args...)) {
        return [func](Args... args) -> decltype(auto) {
            return func(std::forward<Args>(args)...);
        };
    }

} // namespace overload_detail

template <typename... Funcs>
auto make_overload(Funcs&&... funcs) {
    using overload_detail::to_functor;
    using overload_detail::Overload;

    return Overload<std::decay_t<decltype(
            to_functor(std::forward<Funcs>(funcs)))>...>{
        to_functor(std::forward<Funcs>(funcs))...};
}
} // namespace sharp
