#pragma once

#include <sharp/Traits/detail/Overload.hpp>

#include <utility>

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
    class Overload<Func, Funcs...> : public Func, public Overload<Funcs...> {
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
        explicit Overload(F&& f) : Func{std::forward<F>(f)} {}

        using Func::operator();
    };

} // namespace overload_detail

template <typename... Funcs>
auto make_overload(Funcs&&... funcs) {
    return Overload<std::decay_t<Funcs>...>{std::forward<Funcs>(funcs)...};
}
} // namespace sharp
