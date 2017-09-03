#pragma once

#include <sharp/Overload/Overload.hpp>
#include <sharp/Traits/Traits.hpp>

#include <utility>
#include <type_traits>
#include <tuple>

namespace sharp {
namespace overload_detail {


    /**
     * return true if the type is a function pointer type
     */
    template <typename Func>
    struct IsFunctionPtr : public std::integral_constant<bool, false> {};
    template <typename ReturnType, typename... Args>
    struct IsFunctionPtr<ReturnType (*) (Args...)>
        : public std::integral_constant<bool, true> {};

    /**
     * Enable if the function is a function pointer type
     */
    template <typename Func>
    using EnableIfNotFunctionPointerType = std::enable_if_t<
        !IsFunctionPtr<std::decay_t<Func>>::value>;

    /**
     * An incccesible integral_constant-ish type that is used to distinguish
     * between user code return types and make pretend return types.  This is
     * used to determine which overload should be preferred
     */
    template <int Value>
    class InaccessibleConstant : public std::integral_constant<int, Value> {};

    /**
     * Check if the type is an instantiation of InaccessibleConstant
     */
    template <typename T>
    struct IsInstantiationOfInaccessibleConstant
            : public std::integral_constant<bool, false> {};
    template <int Value>
    struct IsInstantiationOfInaccessibleConstant<InaccessibleConstant<Value>>
            : public std::integral_constant<bool, true> {};

    /**
     * The overload detector for function pointers
     */
    template <int current, typename... TypeList>
    class FunctionOverloadDetector;
    template <int current, typename Func, typename... Tail>
    class FunctionOverloadDetector<current, std::tuple<Func, Tail...>>
            : public Func,
            public FunctionOverloadDetector<current, std::tuple<Tail...>> {
    public:

        /**
         * Declare the current impl to just be using the operator() of the
         * functor, since that will never be able to return an
         * InaccessibleConstant this can be used to distinguish between
         * whether a function is being called or a functor
         */
        using Func::operator();
        using FunctionOverloadDetector<current, std::tuple<Tail...>>::operator();
    };
    template <int current,
              typename ReturnType, typename... Args,
              typename... Tail>
    class FunctionOverloadDetector<current, std::tuple<ReturnType (*) (Args...), Tail...>>
            : public FunctionOverloadDetector<current + 1, std::tuple<Tail...>> {
    public:

        /**
         * Declare the current impl, use the return type as a guide for which
         * overload was called
         *
         * This will return an inaccessible constant which will be used to
         * determine whether or not a function pointer is the one overload
         * resolution happens to and also determine which function pointer
         * overload resolution dispatches to
         *
         * And then import all the other impl functions as well
         */
        InaccessibleConstant<current> operator()(Args...);
        using FunctionOverloadDetector<current + 1, std::tuple<Tail...>>::operator();
    };
    template <int current>
    class FunctionOverloadDetector<current, std::tuple<>> {
    private:
        class Inaccessible{};
    public:
        InaccessibleConstant<current> operator()(Inaccessible);
    };

    /**
     * Enable if the return type of overload detector is an instantiation
     * of InaccessibleConstant
     */
    template <typename OverloadDetector, typename... Ts>
    using EnableIfFunctionPointerBetterOverload = std::enable_if_t<
        IsInstantiationOfInaccessibleConstant<
            decltype(std::declval<OverloadDetector>()(
                        std::declval<Ts>()...))>::value>;

    /**
     * The base overload template that is an incomplete type, this should
     * never be instantiated.  And even if it is, it should cause a hard error
     */
    template <typename OverloadDetector, typename... Funcs>
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
    template <typename OverloadDetector, typename Func, typename... Funcs>
    class Overload<OverloadDetector, Func, Funcs...>
            : public Func,
              public Overload<OverloadDetector, Funcs...> {
    public:
        template <typename F, typename... Fs>
        explicit Overload(F&& f, Fs&&... fs)
            : Func{std::forward<F>(f)},
            Overload<OverloadDetector, Funcs...>{std::forward<Fs>(fs)...} {}

        /**
         * Import operator() of the current functor, whatever that may be
         * based on the type of Func and then recurse and import the
         * operator() aggregated recursively
         */
        using Func::operator();
        using Overload<OverloadDetector, Funcs...>::operator();
    };

    template <typename OverloadDetector, typename Func>
    class Overload<OverloadDetector, Func> : public Func {
    public:
        template <typename F>
        explicit Overload(F&& f) : Func{std::forward<F>(f)} {}

        using Func::operator();
    };

    /**
     * Overload cases for function pointer types, this makes a list of the
     * function pointer types and checks if any of the function pointers are
     * callable with the types passed into the function, and if they are then
     * a template function that accepts a variadic list of forwarding
     * references is instantiated and the arguments passed to the appropriate
     * overload
     *
     * TODO This also needs to check the return type of the functors and make
     * sure that they are not the ones being called by enabling the variadic
     * function above only if the return type of the overload being called is
     * an instantiation of InaccessibleConstant<>, to do this the first thing
     * that needs to be done before the overloads are instantiated is to
     * generate a OverloadDetector with all the types and then pass that
     * overload detector to the Overload class
     */
    template <typename OverloadDetector,
              typename ReturnTypeOne, typename... ArgsOne,
              typename ReturnTypeTwo, typename... ArgsTwo, typename... Tail>
    class Overload<OverloadDetector, ReturnTypeOne (*) (ArgsOne...),
                   ReturnTypeTwo (*) (ArgsTwo...), Tail...> {
    public:

        using HeadOne = ReturnTypeOne (*) (ArgsOne...);
        using HeadTwo = ReturnTypeTwo (*) (ArgsTwo...);

        template <typename... FPtrs>
        explicit Overload(FPtrs&&... fs) {

            // store the function pointers
            using FPtrTupleType = decltype(this->function_pointers);
            this->function_pointers = FPtrTupleType{fs...};
        }

        /**
         * Enable this function that is essentially a catch all only if a
         * function is a better call as opposed to a lambda, and if that is
         * the case then the lambda should never be called because a template
         * will always be a better candidate.  And when this template function
         * is called, the right function will be dispatched to
         */
        template <typename... Ts,
                  EnableIfFunctionPointerBetterOverload<OverloadDetector,
                                                        Ts...>* = nullptr>
        decltype(auto) operator()(Ts&&... args) {

            // get the index with the function overload detector
            using IndexType = decltype(std::declval<OverloadDetector>()(
                        std::declval<Ts>()...));

            // and then call the appropriate function
            return std::get<IndexType::value>(this->function_pointers)(
                    std::forward<Ts>(args)...);
        }

    private:

        /**
         * All the function pointers stored here
         */
        std::tuple<HeadOne, HeadTwo, Tail...> function_pointers;
    };
    /**
     * Base case two, when there is only one function pointer, this is needed
     * to solve some ambiguity
     */
    template <typename OverloadDetector, typename ReturnType, typename... Args>
    class Overload<OverloadDetector, ReturnType (*) (Args...)> {
    public:

        using FPtr_t = ReturnType (*) (Args...);

        template <typename F>
        explicit Overload(F&& f) : f_ptr{std::forward<FPtr_t>(f)} {}

        template <typename... Ts,
                  EnableIfFunctionPointerBetterOverload<OverloadDetector,
                                                        Ts...>* = nullptr>
        decltype(auto) operator()(Ts&&... args) {

            // and then call the appropriate function
            return this->f_ptr(std::forward<Args>(args)...);
        }

    private:
        /**
         * Store the single function pointer
         */
        FPtr_t f_ptr;
    };

    /**
     * A trait that gets a value list corresponding to all the functors in the
     * type list
     */
    /**
     * Base case
     */
    template <typename FunctorVList, typename FPtrVList, int current,
              typename... Funcs>
    struct SplitLists {
        static_assert(current >= 0, "");
        using type = std::pair<FunctorVList, FPtrVList>;
    };
    template <typename FunctorVList, typename FPtrVList,
              int current,
              typename Head, typename... Funcs>
    struct SplitLists<FunctorVList, FPtrVList, current, Head, Funcs...> {
        static_assert(current >= 0, "");

        // in the default case the head is a functor so concatenate the empty
        // value list with 0
        using NewFunctorVList = Concatenate_t<FunctorVList, ValueList<current>>;
        using NewFPtrVList = FPtrVList;

        using Next = SplitLists<NewFunctorVList, NewFPtrVList, current + 1,
                                Funcs...>;
        using type = typename Next::type;
    };
    /**
     * Specialization for function pointers
     */
    template <typename FunctorVList, typename FPtrVList,
              int current,
              typename ReturnType, typename... Args, typename... Funcs>
    struct SplitLists<FunctorVList, FPtrVList, current,
                      ReturnType (*) (Args...), Funcs...> {
        static_assert(current >= 0, "");

        // in the default case the head is a functor so concatenate the empty
        // value list with 0
        using NewFunctorVList = FunctorVList;
        using NewFPtrVList = Concatenate_t<FPtrVList, ValueList<current>>;

        using Next = SplitLists<NewFunctorVList, NewFPtrVList, current + 1,
                                Funcs...>;
        using type = typename Next::type;
    };

    /**
     * A trait that gets a value list corresponding to all the function
     * pointers in the type list
     */
    template <typename...> struct WhichType;
    template <typename TupleType>
    struct SplitFunctorAndFunctionPointers;
    template <typename... Args>
    struct SplitFunctorAndFunctionPointers<std::tuple<Args...>> {

        /**
         * split the lists into functor value lists and function pointer value
         * lists
         */
        using SplitValueLists = typename SplitLists<
            ValueList<>, ValueList<>, 0, std::decay_t<Args>...>::type;

        // get the individual value lists from the aggregate type returned
        using FunctorVList = typename SplitValueLists::first_type;
        using FPtrVList = typename SplitValueLists::second_type;

        template <typename TupleType>
        static auto impl(TupleType&& tup) {
            return impl(std::forward<TupleType>(tup), FunctorVList{},
                    FPtrVList{});
        }

    private:
        template <typename TupleType, int... IndicesFunctor, int... IndicesFPtr>
        static auto impl(TupleType&& tup,
                  ValueList<IndicesFunctor...>,
                  ValueList<IndicesFPtr...>) {

            using TupleTypeDecayed = std::decay_t<TupleType>;
            using TupleTypeToReturn = std::tuple<
                std::tuple_element_t<IndicesFunctor, TupleTypeDecayed>...,
                std::tuple_element_t<IndicesFPtr, TupleTypeDecayed>...>;

            // use std::move to aid in reference collapsing and keep the
            // referencesness of the tuple element type
            //
            // this works because the && transformation is a identity
            // transformation, i.e.
            //  int&& && -> int&&,
            //  int& && -> int&,
            return TupleTypeToReturn{
                std::get<IndicesFunctor>(std::move(tup))...,
                std::get<IndicesFPtr>(std::move(tup))...};
        }
    };

    template <typename... FuncArgs, int... Indices>
    auto make_overload_impl(std::tuple<FuncArgs...>& args,
                            std::integer_sequence<int, Indices...>) {

        // generate an overload detector type that will be used to determine
        // whether or not a function pointer will be dispatched to and if so
        // which function pointer in the list that will be
        using OverloadDetector = FunctionOverloadDetector<
            0, std::tuple<std::decay_t<FuncArgs>...>>;

        // call the overload impl, which will detect the type of function
        // argument and then dispatch to the appropriate function
        return overload_detail::Overload<OverloadDetector,
                                         std::decay_t<FuncArgs>...>{
            std::get<Indices>(std::move(args))...};
    }

} // namespace overload_detail

template <typename... Funcs>
auto make_overload(Funcs&&... funcs) {
    // pack the arguments in a tuple
    auto args = std::forward_as_tuple(std::forward<Funcs>(funcs)...);
    using FuncTypes = decltype(args);

    // split the arguments based on whether they are function pointers or
    // functors, with function pointers being at the end
    auto functor_function_ptrs =
        overload_detail::SplitFunctorAndFunctionPointers<FuncTypes>::impl(args);

    constexpr auto size = std::tuple_size<FuncTypes>::value;
    return overload_detail::make_overload_impl(functor_function_ptrs,
            std::make_integer_sequence<int, size>{});
}

} // namespace sharp
