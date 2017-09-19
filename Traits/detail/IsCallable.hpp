/**
 * @file IsCallable.hpp
 * @author Aaryaman Sagar
 *
 * This file contains a trait that can be used to check if an instance of any
 * type can be treated as a callable.  The declaration of the callable does
 * not matter, i.e. the number of parameters the types of parameters and the
 * return type of the callable does not matter.  Further it also does not
 * matter if the callable type has functor overloads or is templated, as long
 * as it has an operator() method in there somewhere this trait will do the
 * right thing
 */

#pragma once

#include <type_traits>

namespace sharp {

namespace detail {
    /**
     * Concepts-ish
     */
    /**
     * Enable if the type is a fundamental type
     */
    template <typename Func>
    using EnableIfFundamental = std::enable_if_t<
        std::is_fundamental<Func>::value>;
    /**
     * Enable if the type is a function pointer
     */
    template <typename Func>
    using EnableIfFunction = std::enable_if_t<
        std::is_function<std::remove_pointer_t<std::decay_t<Func>>>::value>;
    /**
     * Enable if the type is a member function pointer
     */
    template <typename Func>
    using EnableIfMemberFunction = std::enable_if_t<
        std::is_member_function_pointer<std::decay_t<Func>>::value>;

    /**
     * A functor to check if a passed in type is a functor
     *
     * It uses the member detection axiom to check for the presence of a
     * member.
     *
     * Unlike most SFINAE based detection schemes this does not directly check
     * for the presence of a member function (an operator()), but this does a
     * reverse check.  It introduces an operator() in a class that inherits
     * from the callable that we need to check for and then it checks to see
     * if there is an ambiguity in the operator() declaration types, and if
     * there is then the functor knows that there is an operator() in there
     * somewhere that is colliding with the one we defined, and thereby
     * determining that the member operator() must be defined in there
     * somewhere.  Clever!
     */
    template <typename IsFunc>
    struct IsCallableFunctor {
    private:
        /**
         * The two classes I talked about in the comments above, one contains
         * the injected operator() that will be tested against for ambiguity
         * and another that will inherit from the functor (or not a functor)
         * that was passed in.
         *
         * The second erases the type of the functor by inheriting from it,
         * though the type erasure by itself is not imporant here.
         */
        struct BaseFunctor {
            void operator()() {}
        };
        template <typename IsFuncImpl>
        struct DerivedFunctor : IsFuncImpl, BaseFunctor {};

        template <typename IsFuncImpl, typename = std::enable_if_t<true>>
        struct IsCallableFunctorImpl : std::integral_constant<bool, true> {};

        template <typename IsFuncImpl>
        struct IsCallableFunctorImpl<IsFuncImpl, std::enable_if_t<std::is_same<
                decltype(&DerivedFunctor<IsFuncImpl>::operator()),
                decltype(&BaseFunctor::operator())>::value>>
            : std::integral_constant<bool, false> {};

    public:
        static constexpr const bool value
            = IsCallableFunctorImpl<IsFunc>::value;
    };

} // namespace detail

/**
 * This functor aggregates the results from the std::is_function type trait
 * that checks to see if the passed in thing is a plain old function or a
 * functor
 */
template <typename Func, typename = std::enable_if_t<true>>
struct IsCallable {
    static constexpr const bool value = detail::IsCallableFunctor<Func>::value;
};

/**
 * This is needed when the type is a fundamental type like an int.  If this
 * were not there then the specialization of IsCallable that an int would go
 * to is the first one, which assumes that the type is a class type, and
 * inheriting from a class type is an error, therefore there would be an error
 * without this specialization
 */
template <typename Func>
struct IsCallable<Func, detail::EnableIfFundamental<Func>>
    : std::integral_constant<bool, false> {};

/**
 * Remove the pointer type and then partially specialize in the case where the
 * argument is a function type, either a function pointer or a pure function
 * type
 */
template <typename Func>
struct IsCallable<Func, detail::EnableIfFunction<Func>>
    : std::integral_constant<bool, true> {};

template <typename Func>
struct IsCallable<Func, detail::EnableIfMemberFunction<Func>>
    : std::integral_constant<bool, true> {};

/**
 * Convenience variable template for uniformity with the standard library
 * value traits, the _v is added to all value traits in the C++17 standard
 */
template <typename Func>
constexpr bool IsCallable_v = IsCallable<Func>::value;

} // namespace sharp
