#pragma once

#include <sharp/Overload/Overload.hpp>
#include <sharp/Traits/Traits.hpp>

#include <utility>
#include <type_traits>
#include <tuple>

namespace sharp {
namespace overload_detail {


    /**
     * An incccesible integral_constant-ish type that is used to distinguish
     * between user code return types and make pretend return types.
     *
     * Since lambdas in user space cannot return this.  This is used as the
     * return type for function pointer calls when constructing a list of
     * possible function calls.  If overload resolution dispatches to
     * something that returns an instance of InaccessibleConstant then it has
     * to be a function pointer.
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
     * The overload detector, all this does is import the list of function
     * calls in each type, if function type is a functor then operator() will
     * be imported, and if the type is a function type the return type will be
     * changed to InaccessibleConstant and a pretend operator() will be
     * created
     */
    template <int current, typename... TypeList>
    class FunctionOverloadDetector;
    /**
     * Specialziation for function object types
     */
    template <int current, typename Func, typename... Tail>
    class FunctionOverloadDetector<current, Func, Tail...>
            : public Func,
            public FunctionOverloadDetector<current, Tail...> {
    public:

        /**
         * Declare the current impl to just be using the operator() of the
         * functor, since that will never be able to return an
         * InaccessibleConstant this can be used to distinguish between
         * whether a function is being called or a functor
         */
        using Func::operator();
        using FunctionOverloadDetector<current, Tail...>::operator();
    };
    /**
     * Specialization for function pointer types
     */
    template <int current,
              typename ReturnType, typename... Args,
              typename... Tail>
    class FunctionOverloadDetector<current, ReturnType (*) (Args...), Tail...>
            : public FunctionOverloadDetector<current + 1, Tail...> {
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
        using FunctionOverloadDetector<current + 1, Tail...>::operator();
    };
    /**
     * Base case just creates a never callable operator()
     */
    template <int current>
    class FunctionOverloadDetector<current> {
    protected:

        /**
         * This is never callable as the user can never supply an Inaccessible
         * argument (or more like should not)
         *
         * Further the class Inaccessible is not a complete type, so it can
         * never be instantiated in any code
         *
         * Also the member is protected so is not visible in user score
         */
        class Inaccessible;
        InaccessibleConstant<current> operator()(Inaccessible);
    };

    /**
     * Enable if the return type of overload detector is an instantiation
     * of InaccessibleConstant
     */
    template <typename OverloadDetector, typename... Ts>
    using EnableIfFunctionPointerBetterOverload = std::enable_if_t<
        IsInstantiationOfInaccessibleConstant<
            decltype(std::declval<OverloadDetector>()(std::declval<Ts>()...))>
            ::value>;

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
     * TODO Maybe add a check here that only enables the operator() of the
     * functors if they are not the ones being overload resolved to.  In
     * all(most?) cases when the functors are not being preferred over a
     * function pointer below, then the variadic forwarding reference template
     * should be a better match and nothing tricky should happen, but make
     * sure of this and maybe just add in an EnableIfFunctorPreferred to the
     * operator() of the functors and make the using operator() in the functor
     * specializations private.  This will require a layer of CRTP over all
     * the overloads so that the base classes (i.e. the more terminating
     * specialization) can import things from the classes above them.  And I
     * am not sure whether it will work
     */
    template <typename OverloadDetector,
              typename ReturnType, typename... Args, typename... Tail>
    class Overload<OverloadDetector, ReturnType (*) (Args...), Tail...> {
    public:

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
        std::tuple<ReturnType (*) (Args...), Tail...> function_pointers;
    };

    /**
     * Base case two, when there is only one function pointer, this is needed
     * to solve some ambiguity
     */
    template <typename OverloadDetector, typename ReturnType, typename... Args>
    class Overload<OverloadDetector, ReturnType (*) (Args...)> {
    public:

        using FPtr_t = ReturnType (*) (Args...);

        /**
         * Constructor stores the function pointer
         */
        template <typename F>
        explicit Overload(F&& f) : f_ptr{std::forward<FPtr_t>(f)} {}

        /**
         * Enable this function only if the function pointers are a better
         * match than any of the lambdas
         */
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
            0, std::decay_t<FuncArgs>...>;

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
