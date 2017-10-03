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
     * something that returns an instance of FPtrConstant then it has
     * to be a function pointer.
     */
    template <int Value>
    class FPtrConstant : public std::integral_constant<int, Value> {};
    template <int Value>
    class MemberFPtrConstant : public std::integral_constant<int, Value> {};

    /**
     * Check if the type is an instantiation of FPtrConstant
     */
    template <typename T>
    struct IsInstantiationOfFPtrConstant
            : public std::integral_constant<bool, false> {};
    template <int Value>
    struct IsInstantiationOfFPtrConstant<FPtrConstant<Value>>
            : public std::integral_constant<bool, true> {};
    template <typename T>
    struct IsInstantiationOfMemberFPtrConstant
            : public std::integral_constant<bool, false> {};
    template <int Value>
    struct IsInstantiationOfMemberFPtrConstant<MemberFPtrConstant<Value>>
            : public std::integral_constant<bool, true> {};

    /**
     * Invoke a member function pointer
     */
    template <typename MemFunc, typename Instance, typename... Args>
    decltype(auto) invoke_mem_func(MemFunc func, Instance&& instance,
                                   Args&&... args) {
        return (std::forward<Instance>(instance).*func)(
                    std::forward<Args>(args)...);
    }
    /**
     * The overload detector, all this does is import the list of function
     * calls in each type, if function type is a functor then operator() will
     * be imported, and if the type is a function type the return type will be
     * changed to FPtrConstant and a pretend operator() will be
     * created
     */
    template <int current_fptr, int current_mfptr, typename... Funcs>
    class FunctionOverloadDetector;
    /**
     * Specialziation for function object types, dont increment the counter
     * because the counter is used for indexing into a tuple of function
     * pointers and then getting the right function pointer to call and all
     * the function pointers are going to be together with no functor in
     * between or after them in the tail
     */
    template <int current_fptr, int current_mfptr, typename Func,
              typename... Tail>
    class FunctionOverloadDetector<current_fptr, current_mfptr, Func, Tail...>
            : public Func,
            public FunctionOverloadDetector<current_fptr, current_mfptr,
                                            Tail...> {
    public:

        /**
         * Declare the current impl to just be using the operator() of the
         * functor, since that will never be able to return an
         * FPtrConstant this can be used to distinguish between
         * whether a function is being called or a functor
         */
        using Func::operator();
        using FunctionOverloadDetector<current_fptr, current_mfptr, Tail...>
            ::operator();
    };
    /**
     * Specialization for function pointer types
     */
    template <int current_fptr, int current_mfptr,
              typename ReturnType, typename... Args,
              typename... Tail>
    class FunctionOverloadDetector<current_fptr, current_mfptr,
                                   ReturnType (*) (Args...), Tail...>
            : public FunctionOverloadDetector<current_fptr + 1, current_mfptr,
                                              Tail...> {
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
        FPtrConstant<current_fptr> operator()(Args...) const;
        using FunctionOverloadDetector<current_fptr + 1, current_mfptr, Tail...>
            ::operator();
    };

    /**
     * Specializations for member function pointers, unfortunately all these
     * are needed
     *
     * I probably need to write twice these many for volatile but who needs that
     */
    template <int current_fptr, int current_mfptr,
              typename Return, typename Class, typename... Args,
              typename... Tail>
    class FunctionOverloadDetector<current_fptr, current_mfptr,
                                   Return (Class::*) (Args...), Tail...>
            : public FunctionOverloadDetector<current_fptr, current_mfptr + 1,
                                              Tail...> {
    public:
        using C = std::decay_t<Class>;
        MemberFPtrConstant<current_mfptr> operator()(C&, Args...) const;
        MemberFPtrConstant<current_mfptr> operator()(C&&, Args...) const;
        using FunctionOverloadDetector<current_fptr, current_mfptr + 1, Tail...>
            ::operator();
    };
    template <int current_fptr, int current_mfptr,
              typename Return, typename Class, typename... Args,
              typename... Tail>
    class FunctionOverloadDetector<current_fptr, current_mfptr,
                                   Return (Class::*) (Args...) const, Tail...>
            : public FunctionOverloadDetector<current_fptr, current_mfptr + 1,
                                              Tail...> {
    public:
        using C = const std::decay_t<Class>;
        MemberFPtrConstant<current_mfptr> operator()(C&, Args...) const;
        MemberFPtrConstant<current_mfptr> operator()(C&&, Args...) const;
        using FunctionOverloadDetector<current_fptr, current_mfptr + 1, Tail...>
            ::operator();
    };
    template <int current_fptr, int current_mfptr,
              typename Return, typename Class, typename... Args,
              typename... Tail>
    class FunctionOverloadDetector<current_fptr, current_mfptr,
                                   Return (Class::*) (Args...) &, Tail...>
            : public FunctionOverloadDetector<current_fptr, current_mfptr + 1,
                                              Tail...> {
    public:
        using C = std::decay_t<Class>&;
        MemberFPtrConstant<current_mfptr> operator()(C, Args...) const;
        using FunctionOverloadDetector<current_fptr, current_mfptr + 1, Tail...>
            ::operator();
    };
    template <int current_fptr, int current_mfptr,
              typename Return, typename Class, typename... Args,
              typename... Tail>
    class FunctionOverloadDetector<current_fptr, current_mfptr,
                                   Return (Class::*) (Args...) const &,
                                   Tail...>
            : public FunctionOverloadDetector<current_fptr, current_mfptr + 1,
                                              Tail...> {
    public:
        using C = const std::decay_t<Class>&;
        MemberFPtrConstant<current_mfptr> operator()(C, Args...) const;
        using FunctionOverloadDetector<current_fptr, current_mfptr + 1,
                                       Tail...>::operator();
    };
    template <int current_fptr, int current_mfptr,
              typename Return, typename Class, typename... Args,
              typename... Tail>
    class FunctionOverloadDetector<current_fptr, current_mfptr,
                                   Return (Class::*) (Args...) &&,
                                   Tail...>
            : public FunctionOverloadDetector<current_fptr, current_mfptr + 1,
                                              Tail...> {
    public:
        using C = std::decay_t<Class>&&;
        MemberFPtrConstant<current_mfptr> operator()(C, Args...) const;
        using FunctionOverloadDetector<current_fptr, current_mfptr + 1, Tail...>
            ::operator();
    };
    template <int current_fptr, int current_mfptr,
              typename Return, typename Class, typename... Args,
              typename... Tail>
    class FunctionOverloadDetector<current_fptr, current_mfptr,
                                   Return (Class::*) (Args...) const &&,
                                   Tail...>
            : public FunctionOverloadDetector<current_fptr, current_mfptr + 1,
                                              Tail...> {
    public:
        using C = const std::decay_t<Class>&&;
        MemberFPtrConstant<current_mfptr> operator()(C, Args...) const;
        using FunctionOverloadDetector<current_fptr, current_mfptr + 1, Tail...>
            ::operator();
    };

    /**
     * Base case just creates a never callable operator()
     */
    template <int current_fptr, int current_mfptr>
    class FunctionOverloadDetector<current_fptr, current_mfptr> {
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
        FPtrConstant<current_fptr> operator()(Inaccessible) const;
    };

    /**
     * Enable if the return type of overload detector is an instantiation
     * of FPtrConstant
     */
    template <typename Detector, int Index, typename... Ts>
    using EnableIfThisFunctionPointerBestOverload = std::enable_if_t<
        std::is_same<
            decltype(std::declval<Detector>()(std::declval<Ts>()...)),
            FPtrConstant<Index>>::value>;
    /**
     * Enable if a function pointer type is the type preferred in overload
     * dispatch
     */
    template <typename Detector, typename... Args>
    using EnableIfFunctionPreferred = std::enable_if_t<
        IsInstantiationOfFPtrConstant<
            decltype(std::declval<Detector>()(std::declval<Args>()...))>
        ::value>;
    /**
     * Enable if a functor overload is preferred, if the overload dispatch
     * does not return an instantiation of FPtrConstant then a functor
     * overload will be preferred
     */
    template <typename Detector, typename... Args>
    using EnableIfFunctorPreferred = std::enable_if_t<
        !IsInstantiationOfFPtrConstant<
            decltype(std::declval<Detector>()(std::declval<Args>()...))>
        ::value>;
    /**
     * Enable if a member function pointer type is the type preferred in
     * overload dispatch
     */
    template <typename Detector, typename... Args>
    using EnableIfMemFuncPreferred = std::enable_if_t<
        IsInstantiationOfMemberFPtrConstant<
            decltype(std::declval<Detector>()(std::declval<Args>()...))>
        ::value>;
    /**
     * Enable if the type is not an instantiation of self, this is used to
     * prevent forwarding reference constructors from matching everything
     */
    template <typename Type>
    using EnableIfTuple =  sharp::void_t<
        decltype(std::get<0>(std::declval<Type>()))>;
    template <typename Type, typename Self>
    using EnableIfNotSelf = std::enable_if_t<!std::is_same<
        std::decay_t<Type>, std::decay_t<Self>>::value>;
    /**
     * Check to see if the overload resolution is well defined
     */
    template <typename Detector, typename... Args>
    constexpr const auto overload_well_formed = std::is_same<
        sharp::void_t<decltype(std::declval<Detector>()(
                    std::declval<Args>()...))>,
        void>::value;

    /**
     * A trait that gets a value list corresponding to all the functors in the
     * type list
     */
    /**
     * Base case
     */
    template <typename FunctorVList, typename FPtrVList, typename MemFPtrVList,
              int current, typename T>
    struct SplitLists;
    template <typename FunctorVList, typename FPtrVList, typename MemFPtrVList,
              int current>
    struct SplitLists<FunctorVList, FPtrVList, MemFPtrVList, current,
                      ValueList<>> {
        static_assert(current >= 0, "");
        using type = std::tuple<FunctorVList, FPtrVList, MemFPtrVList>;
    };
    template <typename FunctorVList, typename FPtrVList, typename MemFPtrVList,
              int current, int... Types>
    struct SplitLists<FunctorVList, FPtrVList, MemFPtrVList,
                      current, ValueList<FUNCTOR, Types...>> {
        static_assert(current >= 0, "");

        // in the default case the head is a functor so concatenate the empty
        // value list with 0
        using NewFunctorVList = Concatenate_t<FunctorVList, ValueList<current>>;
        using NewMemFPtrVlist = MemFPtrVList;
        using NewFPtrVList = FPtrVList;

        using Next = SplitLists<NewFunctorVList, NewFPtrVList, NewMemFPtrVlist,
                                current + 1,
                                ValueList<Types...>>;
        using type = typename Next::type;
    };
    /**
     * Specialization for function pointers
     */
    template <typename FunctorVList, typename FPtrVList, typename MemFPtrVList,
              int current, int... Types>
    struct SplitLists<FunctorVList, FPtrVList, MemFPtrVList,
                      current, ValueList<F_PTR, Types...>> {
        static_assert(current >= 0, "");

        // in the default case the head is a functor so concatenate the empty
        // value list with 0
        using NewFunctorVList = FunctorVList;
        using NewFPtrVList = Concatenate_t<FPtrVList, ValueList<current>>;
        using NewMemFPtrVlist = MemFPtrVList;

        using Next = SplitLists<NewFunctorVList, NewFPtrVList, NewMemFPtrVlist,
                                current + 1,
                                ValueList<Types...>>;
        using type = typename Next::type;
    };
    template <typename FunctorVList, typename FPtrVList, typename MemFPtrVList,
              int current, int... Types>
    struct SplitLists<FunctorVList, FPtrVList, MemFPtrVList,
                      current, ValueList<MEMBER_F_PTR, Types...>> {
        static_assert(current >= 0, "");

        // in the default case the head is a functor so concatenate the empty
        // value list with 0
        using NewFunctorVList = FunctorVList;
        using NewFPtrVList = FPtrVList;
        using NewMemFPtrVlist = Concatenate_t<MemFPtrVList, ValueList<current>>;

        using Next = SplitLists<NewFunctorVList, NewFPtrVList, NewMemFPtrVlist,
                                current + 1,
                                ValueList<Types...>>;
        using type = typename Next::type;
    };

    /**
     * The implementation for function pointers, this contains a forwarding
     * reference variadic operator() that is only enabled if the current
     * function pointer is the best overload
     *
     * In C++17 this can be made a lot simpler
     *
     *      template<class... Ts>
     *      struct overloaded : Ts... {
     *          using Ts::operator()...;
     *      };
     *      template<class... Ts>
     *      overloaded(Ts...) -> overloaded<Ts...>;
     *
     * This makes convenient use of template argument deduction via template
     * arguments and the pack expansion on using statements that does not
     * currently exist
     */
    template <typename Detector, typename... Funcs>
    class OverloadImpl {};
    template <typename Detector, typename Func, typename... Funcs>
    class OverloadImpl<Detector, Func, Funcs...>
            : public Func, public OverloadImpl<Detector, Funcs...> {
    public:

        template <typename F, typename... Fs,
                  EnableIfNotSelf<F, OverloadImpl>* = nullptr>
        explicit OverloadImpl(F&& f, Fs&&... fs)
            : Func{std::forward<F>(f)},
              OverloadImpl<Detector, Funcs...>{
                std::forward<Fs>(fs)...} {}

        using Func::operator();
        using OverloadImpl<Detector, Funcs...>::operator();
    };
    /**
     * Base case, do not inherit from anything and do not import anything
     */
    template <typename Detector, typename Func>
    class OverloadImpl<Detector, Func> : public Func {
    public:
        template <typename F, EnableIfNotSelf<F, OverloadImpl>* = nullptr>
        explicit OverloadImpl(F&& f) : Func{std::forward<F>(f)} {}

        using Func::operator();
    };

    /**
     * Check that the overload detector is valid and then if there is a
     * possible call with the overload detector, dispatch that to either the
     * collection of functors or to the collection of function pointers
     */
    template <typename Detector,
              typename ValueListFunctions, typename ValueListFunctors,
              typename ValueListMemFuncs, typename... Funcs>
    class CheckAndForward;
    template <typename Detector, int... IndicesFunctions,
              int... IndicesFunctors, int... IndicesMemFuncs,
              typename... Funcs>
    class CheckAndForward<Detector,
                          ValueList<IndicesFunctions...>,
                          ValueList<IndicesFunctors...>,
                          ValueList<IndicesMemFuncs...>,
                          Funcs...> {
    public:
        /**
         * Typedefs for the functions and functors that are going to be used
         * to store the function pointers and the functors
         */
        using Functors = OverloadImpl<
            Detector,
            std::tuple_element_t<IndicesFunctors, std::tuple<Funcs...>>...>;
        using Functions = std::tuple<std::decay_t<
            std::tuple_element_t<IndicesFunctions, std::tuple<Funcs...>>>...>;
        using MemFuncs = std::tuple<std::decay_t<
            std::tuple_element_t<IndicesMemFuncs, std::tuple<Funcs...>>>...>;

        /**
         * Only use this constructor when the instance is is a tuple-like
         * thing decomposable using std::get
         */
        template <typename Args, EnableIfTuple<Args>* = nullptr>
        explicit CheckAndForward(Args&& args)
            : functors{std::get<IndicesFunctors>(std::forward<Args>(args))...},
            functions{std::get<IndicesFunctions>(std::forward<Args>(args))...},
            mem_funcs{std::get<IndicesMemFuncs>(std::forward<Args>(args))...}
        {}

        /**
         * The templated function call operator, make sure that one of the
         * function pointers are going to be called and then forward the
         * arguments to the appropriate function pointer, as determined by the
         * function overload detector
         */
        template <typename... Args,
                  EnableIfFunctionPreferred<Detector, Args...>* = nullptr,
                  int Index = decltype(std::declval<Detector>()(
                                       std::declval<Args>()...))::value>
        auto operator()(Args&&... args) const
                -> decltype(std::get<Index>(std::declval<Functions>())(
                            std::declval<Args>()...)) {
            using std::get;
            static_assert(overload_well_formed<const Detector, Args...>, "");
            return get<Index>(this->functions)(std::forward<Args>(args)...);
        }

        /**
         * The templated function call operator, make sure that one of the
         * function pointers are going to be called and then forward the
         * arguments to the appropriate function pointer, as determined by the
         * function overload detector
         */
        template <typename Instance, typename... Args,
                  EnableIfMemFuncPreferred<Detector, Instance, Args...>*
                      = nullptr,
                  int Index = decltype(std::declval<Detector>()(
                                       std::declval<Instance>(),
                                       std::declval<Args>()...))::value>
        decltype(auto) operator()(Instance&& instance, Args&&... args) const {
            using std::get;
            static_assert(overload_well_formed<Detector, Instance, Args...>,"");
            return invoke_mem_func(get<Index>(this->mem_funcs),
                    std::forward<Instance>(instance),
                    std::forward<Args>(args)...);
        }

        /**
         * 4 overloads corresponding to const, non-const lvalue and rvalue
         * overloads for the operator() member function
         */
        template <typename... Args,
                  EnableIfFunctorPreferred<const Detector&&, Args...>*
                      = nullptr>
        auto operator()(Args&&... args) const &&
                -> decltype(std::declval<const Functors&&>()(
                            std::declval<Args>()...)) {
            static_assert(overload_well_formed<const Detector&&, Args...>, "");
            return std::move(this->functors)(std::forward<Args>(args)...);
        }
        template <typename... Args,
                  EnableIfFunctorPreferred<const Detector&, Args...>* = nullptr>
        auto operator()(Args&&... args) const &
                -> decltype(std::declval<const Functors&>()(
                            std::declval<Args>()...)) {
            static_assert(overload_well_formed<const Detector&, Args...>, "");
            return this->functors(std::forward<Args>(args)...);
        }
        template <typename... Args,
                  EnableIfFunctorPreferred<Detector&&, Args...>* = nullptr>
        auto operator()(Args&&... args) &&
                -> decltype(std::declval<Functors&&>()(
                            std::declval<Args>()...)) {
            static_assert(overload_well_formed<Detector&&, Args...>, "");
            return std::move(this->functors)(std::forward<Args>(args)...);
        }
        template <typename... Args,
                  EnableIfFunctorPreferred<Detector&, Args...>* = nullptr>
        auto operator()(Args&&... args) &
                -> decltype(std::declval<Functors&>()(
                            std::declval<Args>()...)) {
            static_assert(overload_well_formed<Detector&, Args...>, "");
            return this->functors(std::forward<Args>(args)...);
        }

        /**
         * The instances of OverloadImpl objects
         */
        Functors functors;
        Functions functions;
        MemFuncs mem_funcs;

        /**
         * The list of argument lists that are provided by this functor
         */
        using ArgumentsList = Concatenate_t<Arguments_t<Funcs>...>;
    };

    /**
     * Checks if the type is an instantiation of CheckAndForward
     */
    template <typename T>
    struct IsInstantiationCheckForward
        : public std::integral_constant<bool, false> {};
    template <typename One, typename Two, typename Three, typename Four,
              typename... Funcs>
    struct IsInstantiationCheckForward<
        CheckAndForward<One, Two, Three, Four, Funcs...>>
        : public std::integral_constant<bool, true> {};
    /**
     * Enable if the type is an instantiation of CheckAndForward
     */
    template <typename Type>
    using EnableIfCheckForward = std::enable_if_t<IsInstantiationCheckForward<
        Type>::value>;

    /**
     * The implementation of the overload function that actually returns the
     * overload object.  This does some template preprocessing on the type
     * lists and creates some stuff required by the CheckAndForward handle
     * class that will be held by the user
     */
    template <typename... Funcs>
    auto overload_impl(std::tuple<Funcs...>&& funcs) {
        // get the overload detector
        using Detector = overload_detail::FunctionOverloadDetector<
            0, 0, std::decay_t<Funcs>...>;

	    // get the value list of the functions and the functors
        using Split = typename overload_detail::SplitLists<
            ValueList<>, ValueList<>, ValueList<>,
            0,
            ValueList<sharp::WhichInvocableType_v<Funcs>...>>::type;
        using FunctorVList = std::tuple_element_t<0, Split>;
        using FPtrVList = std::tuple_element_t<1, Split>;
        using MemFPtrVList = std::tuple_element_t<2, Split>;

        // return the overloaded object
        return overload_detail::CheckAndForward<Detector,
                                                FPtrVList, FunctorVList,
                                                MemFPtrVList,
                                                std::decay_t<Funcs>...>{
            std::move(funcs)};
    }

    /**
     * Flatten a CheckAndForward instance into a tuple of function pointers
     * and a function object, and for the rest just make a tuple only of that
     * element
     */
    template <typename T>
    auto flatten(sharp::preferred_dispatch<0>, T&& instance) {
        return std::forward_as_tuple(std::forward<T>(instance));
    }
    template <typename T, EnableIfCheckForward<std::decay_t<T>>* = nullptr>
    auto flatten(sharp::preferred_dispatch<1>, T&& instance) {

        // get the function pointers and the function object out into tuples
        // and concatenate them
        auto functions = instance.functions;
        auto mem_funcs = instance.mem_funcs;
        auto&& functor = std::forward<T>(instance).functors;
        auto functors = std::forward_as_tuple(
                std::forward<decltype(functor)>(functor));

        return std::tuple_cat(std::move(functions), std::move(functors),
                std::move(mem_funcs));
    }

    /**
     * Individually flatten each argument into a tuple, concatenate those
     * and return the concatenated tuple
     */
    template <std::size_t... Indices, typename TupleParams>
    auto flatten_args(std::index_sequence<Indices...>, TupleParams&& params) {
        auto flattened_args = std::tuple_cat(flatten(
                    sharp::preferred_dispatch<1>{},
                    std::get<Indices>(std::forward<TupleParams>(params)))...);
        return flattened_args;
    }

} // namespace overload_detail

template <typename... Funcs>
auto overload(Funcs&&... funcs) {

    using namespace overload_detail;

    // Flatten any CheckAndForward instances in the arguments into flag
    // arguments that include the functors and the function pointers that
    // compose that instance, this leads to a flag structure of overload
    // resolution and not a very vertical one with lots of inheritance with
    // recursive overload generation
    auto args = flatten_args(
            std::index_sequence_for<Funcs...>{},
            std::forward_as_tuple(std::forward<Funcs>(funcs)...));

    // forward the args to the overload impl function
    return overload_impl(std::move(args));
}

} // namespace sharp
