#pragma once

#include <sharp/Overload/Overload.hpp>

#include <utility>
#include <type_traits>

namespace sharp {
namespace overload_detail {

    /**
     * A helper class that detects the type of function passed in and switches
     * based on the type.  For example if a function is passed in then the
     * operator() is defined by the class manually and the arguments forwarded
     * to the function and if a functor is passed the operator() of the
     * functor is used
     */
    template <typename Func>
    class OverloadFuncImpl : public Func {
    public:
        template <typename F>
        explicit OverloadFuncImpl(F&& f) : Func{std::forward<F>(f)} {}

        /**
         * Import operator()
         */
        using Func::operator();
    };
    template <typename ReturnType, typename... Args>
    class OverloadFuncImpl<ReturnType (*) (Args...)> {
    public:
        template <typename F>
        explicit OverloadFuncImpl(F&& f) : func{std::forward<F>(f)} {}

        using FPtr_t = ReturnType (*) (Args...);

        /**
         * Call the function and forward the return type
         *
         * This only participates in overload resolution if the function
         * arguments can be forwarded properly to the function stored
         */
        ReturnType operator()(Args... args) const {
            return this->func(std::forward<Args>(args)...);
        }

    private:
        /**
         * Store a pointer to the function
         */
        FPtr_t func;
    };

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
            : public OverloadFuncImpl<Func>,
              public Overload<Funcs...> {
    public:
        template <typename F, typename... Fs>
        explicit Overload(F&& f, Fs&&... fs)
            : OverloadFuncImpl<Func>{std::forward<F>(f)},
            Overload<Funcs...>{std::forward<Fs>(fs)...} {}

        /**
         * Import operator() of the current functor, whatever that may be
         * based on the type of Func and then recurse and import the
         * operator() aggregated recursively
         */
        using OverloadFuncImpl<Func>::operator();
        using Overload<Funcs...>::operator();
    };

    /**
     * Base case, just provide a constructor and import the operator() of the
     * current functor type
     */
    template <typename Func>
    class Overload<Func> : public OverloadFuncImpl<Func> {
    public:
        template <typename F>
        explicit Overload(F&& f)
            : OverloadFuncImpl<Func>{std::forward<F>(f)} {}

        /**
         * Import the operator() of the OverloadFuncImpl class, whatever that
         * may be based on the type of Func
         */
        using OverloadFuncImpl<Func>::operator();
    };

} // namespace overload_detail

template <typename... Funcs>
auto make_overload(Funcs&&... funcs) {
    return overload_detail::Overload<std::decay_t<Funcs>...>{
        std::forward<Funcs>(funcs)...};
}
} // namespace sharp
