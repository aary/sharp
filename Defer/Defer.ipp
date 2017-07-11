#pragma once

#include <sharp/Defer/Defer.hpp>

#include <type_traits>
#include <utility>

namespace sharp {

namespace detail {

    /**
     * @class Defer
     *
     * The basic implementation of a releasable defer
     */
    template <typename Func>
    class Defer {
    public:

        /**
         * constructors for the class just initialize the member variable that
         * stores the function with the passed in function
         */
        Defer(const Func& func_in) noexcept(
                std::is_nothrow_copy_constructible<Func>::value)
            : should_execute{true}, func{func_in} {}
        Defer(Func&& func_in) noexcept(
                std::is_nothrow_move_constructible<Func>::value)
            : should_execute{true}, func{std::move(func_in)} {}

        /**
         * Destructor just executes the function on return and nothing else
         */
        ~Defer() {
            if (should_execute) {
                func();
            }
        }

        /**
         * @function reset
         *
         * This method resets the internal state of the deferred function so
         * that it will no longer be executed on destruction or when the scope
         * finishes
         */
        void reset() const noexcept {
            this->should_execute = false;
        }

    private:

        /**
         * Whether or not the function should execute the function stored within
         * it on return
         */
        bool should_execute;

        /**
         * The function instance that will be called unless the deferred
         * function is reset
         */
        Func func;
    };

    template <typename Func>
    class DeferGuard {
    public:

        /**
         * constructors for the class just initialize the member variable that
         * stores the function with the passed in function
         */
        DeferGuard(const Func& func_in) noexcept(
                std::is_nothrow_copy_constructible<Func>::value)
            : func{func_in} {}
        DeferGuard(Func&& func_in) noexcept(
                std::is_nothrow_move_constructible<Func>::value)
            : func{std::move(func_in)} {}

        /**
         * Destructor just executes the function on return and nothing else
         */
        ~DeferGuard() {
            func();
        }

    private:

        /**
         * The function instance that will be called unless the deferred
         * function is reset
         */
        Func func;
    };

} // namespace detail

template <typename Func>
auto defer(Func&& func_in) {
    return detail::Defer<Func>{std::forward<Func>(func_in)};
}

template <typename Func>
auto defer_guard(Func&& func_in) {
    return detail::DeferGuard<Func>{std::forward<Func>(func_in)};
}

} // namespace sharp
