/**
 * @file FunctionIntrospect.hpp
 * @author Aaryaman Sagar
 *
 * This file contains traits that can be used to query the declaration of a
 * function.  This would include querying the return type, the argument types
 * by number and whether an argument of a specific type is present or not in
 * the argument list.
 *
 * This works for both regular functions and any special type of functor that
 * you pass it, as long as the functor has one overload for the function,  if
 * there are multiple overloads for the same function then the type
 * introspection will not work unless you disambiguate the function with a
 * static_cast, there is no way for the trait to tell which function you are
 * talking about
 */

#pragma once

#include <tuple>
#include <type_traits>
#include <cstddef>

#include <sharp/Traits/detail/IsCallable.hpp>
#include <sharp/Traits/detail/TypeLists.hpp>
#include <sharp/Traits/detail/VoidT.hpp>

namespace sharp {

/**
 * MEMBER_F_PTR -> member function pointer
 * F_PTR        -> function pointer
 * FUNCTOR      -> functor
 */
constexpr auto MEMBER_F_PTR = 0;
constexpr auto F_PTR = 1;
constexpr auto FUNCTOR = 2;

/**
 * This type is used when trying to deduce argument types of an overloaded
 * functor, see Arguments_t
 */
struct Unspecified {};

/**
 * This type will be used to represent parameter packs, std::tuple cannot be
 * used because it is used publically and might cause an issue when a function
 * itself accepts a tuple, then the argument types might be misleading in
 * recursive unpacking situations (for example see ConcatenateArgs in
 * sharp/Overload/Overload.upp)
 */
template <typename... Types>
struct Args;

namespace detail {

    /**
     * Enable if the functor has a operator() method which can be
     * introspected, if it has a templated one or if it is an overloaded
     * member function then it cant be introspected
     */
    template <typename Func>
    using EnableIfFunctorNotUnspecified = sharp::void_t<
        decltype(&std::decay_t<Func>::operator())>;
    template <typename Func>
    using EnableIfAlreadyHasArgumentList = sharp::void_t<
        typename std::decay_t<Func>::ArgumentsList>;

    /**
     * The implementation trait for the return type trait, this has two
     * versions - one for regular functions and another for functors, since
     * the two are completely different and have completely different syntax,
     * only one will work for the passed in type
     *
     * These require that the function type passed in is a function pointer
     * type and not a pure function type, therefore the type to query for must
     * be passed in with a std::decay_t or std::add_pointer in the case for
     * functions, but that will not generalize
     */
    /**
     * The first overload is for functors, and the static assertion in the
     * class will make sure that it is initialized only for functors
     */
    template <typename Func>
    struct ReturnTypeImpl {

        /**
         * assert that the type passed into this function is a callable
         */
        static_assert(sharp::IsCallable<Func>::value, "Type must be callable");

        /**
         * Leave the main template unimplemented, the partial specialization
         * will deduce the return type
         */
        template <typename MemberFunctionPointer>
        struct ReturnTypeImplFunctor;
        template <typename ReturnType, typename... Args>
        struct ReturnTypeImplFunctor<ReturnType (Func::*) (Args...)> {
            using type = ReturnType;
        };

        using type = typename ReturnTypeImplFunctor<decltype(&Func::operator())>
            ::type;
    };

    /**
     * These overloads handle the case when the type passed in is of a fucntion
     * type, and then it deduces the return type from the partial
     * specializations
     */
    /**
     * Specialization for when the type of the thing is a function pointer
     * type
     */
    template <typename ReturnType, typename... Args>
    struct ReturnTypeImpl<ReturnType (*) (Args...)> {
        using type = ReturnType;
    };

    /**
     * Implementation trait that will be forwarded a pointer to a member
     * function if Func is a functor, if Func is not a functor then it
     * will go to the other implementation function
     */
    template <typename Func, typename = sharp::void_t<>>
    struct ArgumentsImpl {
        /**
         * An implementation in the case where the functor already contains
         * information about the argument list types it holds
         */
        template <typename F, typename = sharp::void_t<>>
        struct ArgumentsImplImpl {
            using type = sharp::Args<Unspecified>;
        };
        template <typename F>
        struct ArgumentsImplImpl<F, EnableIfAlreadyHasArgumentList<F>> {
            using type = typename std::decay_t<F>::ArgumentsList;
        };

        using type = typename ArgumentsImplImpl<Func>::type;
    };
    template <typename Func>
    struct ArgumentsImpl<Func, EnableIfFunctorNotUnspecified<Func>> {

        /**
         * The implementation trait that is painfully partially specialized to
         * the member function pointer type
         */
        template <typename FuncImpl>
        struct ArgumentsImplFunctor;
        template <typename Functor, typename Return, typename... Args>
        struct ArgumentsImplFunctor<Return (Functor::*) (Args...)> {
            using type = sharp::Args<Args...>;
        };
        template <typename Functor, typename Return, typename... Args>
        struct ArgumentsImplFunctor<Return (Functor::*) (Args...) const> {
            using type = sharp::Args<Args...>;
        };
        template <typename Functor, typename Return, typename... Args>
        struct ArgumentsImplFunctor<Return (Functor::*) (Args...) &> {
            using type = sharp::Args<Args...>;
        };
        template <typename Functor, typename Return, typename... Args>
        struct ArgumentsImplFunctor<Return (Functor::*) (Args...) const &> {
            using type = sharp::Args<Args...>;
        };
        template <typename Functor, typename Return, typename... Args>
        struct ArgumentsImplFunctor<Return (Functor::*) (Args...) &&> {
            using type = sharp::Args<Args...>;
        };
        template <typename Functor, typename Return, typename... Args>
        struct ArgumentsImplFunctor<Return (Functor::*) (Args...) const&&> {
            using type = sharp::Args<Args...>;
        };

        using type = typename ArgumentsImplFunctor<
            decltype(&Func::operator())>::type;
    };
    template <typename ReturnType, typename... Args>
    struct ArgumentsImpl<ReturnType (*) (Args...), sharp::void_t<>> {
        using type = sharp::Args<Args...>;
    };

    template <typename F>
    struct WhichInvocableTypeImpl
            : std::integral_constant<int, FUNCTOR> {};
    template <typename Return, typename... Args>
    struct WhichInvocableTypeImpl<Return (*) (Args...)>
            : std::integral_constant<int, F_PTR> {};
    template <typename Class, typename Return, typename... Args>
    struct WhichInvocableTypeImpl<Return (Class::*) (Args...)>
            : std::integral_constant<int, MEMBER_F_PTR> {};
    template <typename Class, typename Return, typename... Args>
    struct WhichInvocableTypeImpl<Return (Class::*) (Args...) const>
            : std::integral_constant<int, MEMBER_F_PTR> {};
    template <typename Class, typename Return, typename... Args>
    struct WhichInvocableTypeImpl<Return (Class::*) (Args...) &>
            : std::integral_constant<int, MEMBER_F_PTR> {};
    template <typename Class, typename Return, typename... Args>
    struct WhichInvocableTypeImpl<Return (Class::*) (Args...) const &>
            : std::integral_constant<int, MEMBER_F_PTR> {};
    template <typename Class, typename Return, typename... Args>
    struct WhichInvocableTypeImpl<Return (Class::*) (Args...) &&>
            : std::integral_constant<int, MEMBER_F_PTR> {};
    template <typename Class, typename Return, typename... Args>
    struct WhichInvocableTypeImpl<Return (Class::*) (Args...) const &&>
            : std::integral_constant<int, MEMBER_F_PTR> {};

} // namespace detail

/**
 * @class ReturnType
 *
 * Trait that deduces the return type of the passed in callable
 */
template <typename Func>
struct ReturnType {
    static_assert(sharp::IsCallable_v<Func>,
            "Cannot get the return type of a non callable type");
    using type = typename detail::ReturnTypeImpl<std::decay_t<Func>>::type;
};

/**
 * @class Arguments
 *
 * Trait that inspects the function type passed in and returns all the
 * arguments to the function wrapped in a sharp::Args type
 */
template <typename Func>
struct Arguments {
    static_assert(sharp::IsCallable_v<Func>,
            "Can only use sharp::Arguments with callables");

    using type = typename detail::ArgumentsImpl<std::decay_t<Func>>::type;
};

/**
 * @class InvocableType
 *
 * Determines what type of function pointer the thing is, whether a functor, a
 * member function pointer or a plain function
 */
template <typename T>
struct WhichInvocableType : public std::integral_constant<int,
        detail::WhichInvocableTypeImpl<std::decay_t<T>>::value> {};

/**
 * Convenience template for uniformity with the standard library type traits,
 * the _t is added to all type traits in the C++14 standard
 */
template <typename Func>
using ReturnType_t = typename ReturnType<Func>::type;
template <typename Func>
using Arguments_t = typename Arguments<Func>::type;

template <typename Func>
constexpr auto WhichInvocableType_v = WhichInvocableType<Func>::value;

} // namespace sharp

/**
 * Specialize tuple_element_t and tuple_size for Args in the std namespace..
 * *yelp*
 */
namespace std {

    template <std::size_t Index, typename... Types>
    class tuple_element<Index, sharp::Args<Types...>> {
    public:
        using type = std::tuple_element_t<Index, std::tuple<Types...>>;
    };
    template <typename... Types>
    class tuple_size<sharp::Args<Types...>>
        : public std::integral_constant<std::size_t, sizeof...(Types)>
    {};
} // namespace std
