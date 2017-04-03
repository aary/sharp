/**
 * @file TypeAlgorithm.hpp
 * @author Aaryaman Sagar
 *
 * This file contains algorithms that can be executed on a template pack,
 * including things similar to std::any_of std::all_of std::max and so on
 */

#pragma once

#include <type_traits>
#include <tuple>
#include <cstdint>
#include <utility>

#include <sharp/Traits/detail/Functional.hpp>
#include <sharp/Traits/detail/Utility.hpp>
#include <sharp/Traits/detail/IsInstantiationOf.hpp>

namespace sharp {

namespace detail {

    /**
     * Concepts
     */
    /**
     * Enable if the type passed in is an instantiation of std::tuple
     * This trait is used throughout all the algorithms in this module that
     * accept type lists
     */
    template <typename TypeList>
    using EnableIfTuple = std::enable_if_t<
        sharp::IsInstantiationOf_v<TypeList, std::tuple>>;

    /**
     * Implementation for the AllOf trait
     */
    template <template <typename...> class Predicate, typename TypeList>
    struct AllOfImpl {
        static constexpr const bool value = true;
    };
    template <template <typename...> class Predicate,
              typename Head, typename... Tail>
    struct AllOfImpl<Predicate, std::tuple<Head, Tail...>> {
        static constexpr const bool value
            = Predicate<Head>::value
                && detail::AllOfImpl<Predicate, std::tuple<Tail...>>::value;
    };

    /**
     * Implementation for the AnyOf trait, this unlike the AllOf trait returns
     * a false when the list is empty, in line with std::any_of
     */
    template <template <typename...> class Predicate, typename TypeList>
    struct AnyOfImpl {
        static constexpr const bool value = false;
    };
    template <template <typename...> class Predicate,
              typename Head, typename... Tail>
    struct AnyOfImpl<Predicate, std::tuple<Head, Tail...>> {
        static constexpr const bool value
            = Predicate<Head>::value
                || detail::AnyOfImpl<Predicate, std::tuple<Tail...>>::value;
    };

    /**
     * Implemenatation for the ForEach trait
     */
    template <typename TypeList>
    struct ForEachImpl;
    template <typename Head, typename... Tail>
    struct ForEachImpl<std::tuple<Head, Tail...>> {
        template <typename Func>
        void operator()(Func& func) {
            func(sharp::Identity<Head>{});
            ForEachImpl<std::tuple<Tail...>>{}(func);
        };
    };
    template <typename Head>
    struct ForEachImpl<std::tuple<Head>> {
        template <typename Func>
        void operator()(Func func) {
            func(sharp::Identity<Head>{});
        }
    };

    /**
     * Implementation for the count if trait
     */
    template <template <typename...> class Predicate, typename TypeList>
    struct CountIfImpl {
        static constexpr const int value = 0;
    };
    template <template <typename...> class Predicate,
              typename Head, typename... Tail>
    struct CountIfImpl<Predicate, std::tuple<Head, Tail...>> {
        /**
         * convert to an int and add recursively
         */
        static constexpr const int value
            = int(Predicate<Head>::value)
                + CountIfImpl<Predicate, std::tuple<Tail...>>::value;
    };

    /**
     * Implementation for the FindIf trait
     */
    template <template <typename...> class Predicate, typename TypeList>
    struct FindIfImpl {
        using type = std::tuple<>;
    };
    template <template <typename...> class Predicate,
              typename Head, typename... Tail>
    struct FindIfImpl<Predicate, std::tuple<Head, Tail...>> {
        using type = std::conditional_t<
            Predicate<Head>::value,
            std::tuple<Head, Tail...>,
            typename FindIfImpl<Predicate, std::tuple<Tail...>>::type>;
    };

    /**
     * Implementation for the FindFirstOfTrait
     */
    template <typename TypesContainerOne, typename TypesContainerTwo>
    struct FindFirstOfImpl;
    template <typename HeadOne, typename... TailOne,
              typename... TailTwo>
    struct FindFirstOfImpl<std::tuple<HeadOne, TailOne...>,
                           std::tuple<TailTwo...>> {

        /**
         * Iterate through the second list using the first head as what you
         * check for equality, if that returns a false, i.e. if the first
         * element was not in the second list then do the same for the
         * expanded pack on the first list
         *
         * So if the lists are <int, double> and <char, bool> check int to see
         * if int exists in <char, bool>, if it doesnt then check to see if
         * double exists in <char, bool> and so on
         */
        using type = std::conditional_t<!std::is_same<
            typename FindIfImpl<
                       Bind<std::is_same, HeadOne>::template type,
                       std::tuple<TailTwo...>>::type,
            std::tuple<>>::value,
            std::tuple<HeadOne, TailOne...>,
            typename FindFirstOfImpl<std::tuple<TailOne...>,
                                     std::tuple<TailTwo...>>::type>;
    };
    template <typename... TailTwo>
    struct FindFirstOfImpl<std::tuple<>, std::tuple<TailTwo...>> {
        using type = std::tuple<>;
    };

    /**
     * Implementation for the AdjacentFind algorithm
     */
    /**
     * Stop the iteration here, either of the below two cases should be hit in
     * most cases, when they are not hit then the default case will be this
     * one which ends the recursion by defining the type to be the
     * std::tuple<> type
     */
    template <typename TypeList>
    struct AdjacentFindImpl {
        using type = std::tuple<>;
    };
    /**
     * Keep going here, since you have not found types that are the same
     */
    template <typename First, typename Second, typename... Types>
    struct AdjacentFindImpl<std::tuple<First, Second, Types...>> {
        using type = typename AdjacentFindImpl<std::tuple<Second,
                                                          Types...>>::type;
    };
    /**
     * Stop here because you have hit the point where you have found two of
     * the same types
     */
    template <typename First, typename... Types>
    struct AdjacentFindImpl<std::tuple<First, First, Types...>> {
        using type = std::tuple<First, First, Types...>;
    };

    /**
     * Implementation for the Mismatch trait
     */
    template <typename TypesContainerOne, typename TypesContainerTwo>
    struct MismatchImpl;
    template <typename HeadOne, typename... TailOne,
              typename HeadTwo, typename... TailTwo>
    struct MismatchImpl<std::tuple<HeadOne, TailOne...>,
                        std::tuple<HeadTwo, TailTwo...>> {
        using type = std::pair<std::tuple<HeadOne, TailOne...>,
                               std::tuple<HeadTwo, TailTwo...>>;
    };
    template <typename HeadOne, typename... TailOne>
    struct MismatchImpl<std::tuple<HeadOne, TailOne...>, std::tuple<>> {
        using type = std::pair<std::tuple<HeadOne, TailOne...>,
                               std::tuple<>>;
    };
    template <typename HeadTwo, typename... TailTwo>
    struct MismatchImpl<std::tuple<>, std::tuple<HeadTwo, TailTwo...>> {
        using type = std::pair<std::tuple<>,
                               std::tuple<HeadTwo, TailTwo...>>;
    };
    template <>
    struct MismatchImpl<std::tuple<>, std::tuple<>> {
        using type = std::pair<std::tuple<>, std::tuple<>>;
    };
    template <typename HeadOne, typename... TailOne,
              typename... TailTwo>
    struct MismatchImpl<std::tuple<HeadOne, TailOne...>,
                        std::tuple<HeadOne, TailTwo...>> {
        using type = typename MismatchImpl<std::tuple<TailOne...>,
                                           std::tuple<TailTwo...>>::type;
    };

    /**
     * Implementation for the Search trait
     *
     * TODO see if you can reduce the number of base cases here
     */
    template <typename TypesContainerOne, typename TypesContainerTwo>
    struct EqualImpl;
    template <typename HeadOne, typename... TailOne>
    struct EqualImpl<std::tuple<HeadOne, TailOne...>, std::tuple<>> {
        static constexpr const bool value = false;
    };
    template <typename HeadTwo, typename... TailTwo>
    struct EqualImpl<std::tuple<>, std::tuple<HeadTwo, TailTwo...>> {
        static constexpr const bool value = true;
    };
    template <>
    struct EqualImpl<std::tuple<>, std::tuple<>> {
        static constexpr const bool value = true;
    };
    template <typename HeadOne, typename... TailOne,
              typename HeadTwo, typename... TailTwo>
    struct EqualImpl<std::tuple<HeadOne, TailOne...>,
                     std::tuple<HeadTwo, TailTwo...>> {
        static constexpr const bool value = false;
    };
    template <typename HeadOne, typename... TailOne, typename... TailTwo>
    struct EqualImpl<std::tuple<HeadOne, TailOne...>,
                     std::tuple<HeadOne, TailTwo...>> {
        static constexpr const bool value = true && EqualImpl<
            std::tuple<TailOne...>, std::tuple<TailTwo...>>::value;
    };

    /**
     * Implementation for the constexpr max trait
     */
    template <int... integers>
    struct MaxValueImpl : std::integral_constant<int, -1> {};
    template <int first>
    struct MaxValueImpl<first> : std::integral_constant<int, first> {};
    template <int first, int second, int... tail>
    struct MaxValueImpl<first, second, tail...> {
        static constexpr const int value = (first > second)
                ? MaxValueImpl<first, tail...>::value
                : MaxValueImpl<second, tail...>::value;
    };

    /**
     * Implementation for the type based Max trait, this follows the same
     * logic as the trait above, but just differently for types by using the
     * comparator
     */
    template <template <typename...> class Comparator, typename... Types>
    struct MaxTypeImpl {
        using type = std::tuple<>;
    };
    template <template <typename...> class Comparator, typename Head>
    struct MaxTypeImpl<Comparator, Head> {
        using type = Head;
    };
    template <template <typename...> class Comparator,
              typename First, typename Second, typename... Tail>
    struct MaxTypeImpl<Comparator, First, Second, Tail...> {
        using type = std::conditional_t<
            Comparator<First, Second>::value,
            typename MaxTypeImpl<Comparator, Second, Tail...>::type,
            typename MaxTypeImpl<Comparator, First, Tail...>::type>;
    };

    template <int... integers>
    struct MinValueImpl : std::integral_constant<int, -1> {};
    template <int first>
    struct MinValueImpl<first> : std::integral_constant<int, first> {};
    template <int first, int second, int... tail>
    struct MinValueImpl<first, second, tail...> {
        static constexpr const int value = (first < second)
                ? MinValueImpl<first, tail...>::value
                : MinValueImpl<second, tail...>::value;
    };

    /**
     * Implementation for the Search trait
     *
     * TODO remove the third partial specialization and see what happens, the
     * compiler will say that there is an error with type not existing in the
     * template and that is because there is infinite recursion going on
     */
    template <typename TypesContainerOne, typename TypesContainerTwo>
    struct SearchImpl;
    /**
     * Define the base cases
     */
    template <>
    struct SearchImpl<std::tuple<>, std::tuple<>> {
        using type = std::tuple<>;
    };
    template <typename... TypesTwo>
    struct SearchImpl<std::tuple<>, std::tuple<TypesTwo...>> {
        using type = std::tuple<>;
    };
    template <typename... TypesOne>
    struct SearchImpl<std::tuple<TypesOne...>, std::tuple<>> {
        using type = std::tuple<TypesOne...>;
    };
    template <typename HeadOne, typename... TailOne>
    struct SearchImpl<std::tuple<HeadOne, TailOne...>,
                      std::tuple<HeadOne, TailOne...>> {
        using type = std::tuple<HeadOne, TailOne...>;
    };

    /**
     * The big catch all case
     */
    template <typename HeadOne, typename... TailOne,
              typename HeadTwo, typename... TailTwo>
    struct SearchImpl<std::tuple<HeadOne, TailOne...>,
                      std::tuple<HeadTwo, TailTwo...>> {

    private:
        /**
         * find the first occurence of the first thing in the second list in
         * the first list, if this is the result then the type will be
         * typedefed to this, otherwise continue looking in whatever this
         * returned, for example if the second list is <double, char> and the
         * first one is <int, double, double, char> this will first evaluate to
         * <double, double, char> since that is the first occurence of the
         * second list in the first.  Then when that is not satisfactory as
         * the return value, the first list will be popped to be
         * <double, char> (which is the pop_fronted version of the result
         * below
         */
        using find_occurence = typename detail::FindIfImpl<
            Bind<std::is_same, HeadTwo>::template type,
            std::tuple<HeadOne, TailOne...>>::type;

        static_assert(sharp::IsInstantiationOf_v<find_occurence, std::tuple>,
                "Something went wrong in the implementation of "
                "sharp::detail::SearchImpl");

    public:
        /**
         * The human readable pseudocode for the following
         *
         *  Given two lists A and B, find the first occurrence of the first
         *  thing in B in A, name that list find_occurence, for example, if A
         *  is <int, double, double, char> and B is <double, char>, then
         *  find_occurence will be <double, double, char>
         *
         *  If std::equal(B.begin(), B.end(), find_occurence.begin(),
         *                find_occurence.end())
         *      assign the result to be find_occurence
         *  Else if find_occurence == A.end()
         *      assign result = A.end()
         *  Else
         *      // search in the subset of A (find_occurence) by reducing it
         *      // by 1, so in the example above, you will call Search on
         *      // <double, char> instead of <double, double, char>
         *      result = Search(find_occurence + 1, B)
         *
         * If in the end Search was not called on the popped front version of
         * the find_occurence type, then this would continue forever, because
         * if the case was not caught by the two ifs, the last case would keep
         * hitting and the result would be that the compiler would say that
         * the alias type is not found in the infinite recursive call (without
         * actually saying that there was infinite recursion)
         */
        using type = std::conditional_t<EqualImpl<
                std::tuple<HeadTwo, TailTwo...>,
                find_occurence>::value,
            find_occurence,
            std::conditional_t<std::is_same<
                    std::tuple<>,
                    find_occurence>::value,
                std::tuple<>,
                typename SearchImpl<Erase_t<0, find_occurence>,
                                    std::tuple<HeadTwo, TailTwo...>>::type>>;
    };

    /**
     * Implemenattion trait for the searchnimpl trait
     */
    template <typename TypeToRepeat, int n, typename TypeList>
    struct SearchNImpl {
    private:
        /**
         * Get a type list that contains the type to look for repeated n times
         */
        using repeated_type_tuple = ConcatenateN_t<TypeToRepeat, n>;
        static_assert(IsInstantiationOf_v<repeated_type_tuple, std::tuple>,
                "Something went wrong in the implementation of sharp::SearchN");

    public:
        /**
         * Find it in the type list to see if it exists
         */
        using type = typename SearchImpl<TypeList,
                                         repeated_type_tuple>::type;
    };

    /**
     * Implementation for the transform if trait
     */
    template <template <typename...> class Predicate,
              template <typename...> class TransformFunction,
              typename TypeList>
    struct TransformIfImpl {
        using type = std::tuple<>;
    };
    template <template <typename...> class Predicate,
              template <typename...> class TransformFunction,
              typename Head>
    struct TransformIfImpl<Predicate, TransformFunction, std::tuple<Head>> {
        using type = std::conditional_t<Predicate<Head>::value,
            std::tuple<typename TransformFunction<Head>::type>,
            std::tuple<Head>>;
    };
    template <template <typename...> class Predicate,
              template <typename...> class TransformFunction,
              typename Head, typename... Tail>
    struct TransformIfImpl<Predicate, TransformFunction,
                           std::tuple<Head, Tail...>> {
        using type = Concatenate_t<
            typename TransformIfImpl<Predicate,
                                     TransformFunction,
                                     std::tuple<Head>>::type,
            typename TransformIfImpl<Predicate,
                                     TransformFunction,
                                     std::tuple<Tail...>>::type>;
    };

    /**
     * Implementation for the remove if trait
     */
    template <template <typename...> class Predicate, typename TypeList>
    struct RemoveIfImpl {
        using type = std::tuple<>;
    };
    template <template <typename...> class Predicate,
              typename Head, typename... Tail>
    struct RemoveIfImpl<Predicate, std::tuple<Head, Tail...>> {
        /**
         * Recursive pseudocode
         *     type = (predicate(head) ? head : nullptr) | remove_if_impl(tail)
         */
        using type = Concatenate_t<
            std::conditional_t<Predicate<Head>::value,
                std::tuple<>,
                std::tuple<Head>>,
            typename RemoveIfImpl<Predicate, std::tuple<Tail...>>::type>;
    };

    /**
     * Implementation for the reverse trait
     */
    template <typename TypeList>
    struct ReverseImpl {
        using type = std::tuple<>;
    };
    template <typename FirstType, typename... Types>
    struct ReverseImpl<std::tuple<FirstType, Types...>> {
        using type = Concatenate_t<
            typename ReverseImpl<std::tuple<Types...>>::type,
            std::tuple<FirstType>>;
    };

    /**
     * Implementation of the unique trait
     */
    template <typename TypesContainer>
    struct UniqueImpl {
        using type = std::tuple<>;
    };
    template <typename Head, typename... Tail>
    struct UniqueImpl<std::tuple<Head, Tail...>> {
    private:
        /**
         * remove all occurences of the head type from the list and then get
         * the resulting list
         */
        using remove_all_head_from_list_tuple
            = typename RemoveIfImpl<Bind<std::is_same, Head>::template type,
                                    std::tuple<Tail...>>::type;
    public:
        using type = Concatenate_t<
            std::tuple<Head>,
            typename UniqueImpl<remove_all_head_from_list_tuple>::type>;
    };

    /**
     * Implementation of the sort trait, the implementation uses a selection
     * sort, so as a result it may not be stable, similar priority elements
     * might be out of order in the result as compared to what was seen in the
     * initial type list
     */
    template <template <typename...> class Comparator, typename TypesContainer>
    struct SortImpl;
    template <template <typename...> class Comparator>
    struct SortImpl<Comparator, std::tuple<>> {
        using type = std::tuple<>;
    };
    template <template <typename...> class Comparator, typename... Types>
    struct SortImpl<Comparator, std::tuple<Types...>> {
    private:
        /**
         * Get the smallest value, using the comparator and then the
         * MinType trait to get the minimum value, after getting the min type
         * get the location of the min type, erase it from that location and
         * then move it to the front of the tuple to be constructed and then
         * recurse
         *
         * Assert that calling find to get the minimum type is not an empty
         * tuple, because there should always be a smallest type
         */
        using MinType = typename MaxTypeImpl<Negate<Comparator>::template type,
                                             Types...>::type;
        using TypeListWithMinTypeFirst
            = typename FindIfImpl<Bind<std::is_same, MinType>::template type,
                                  std::tuple<Types...>>::type;
        static_assert(!std::is_same<TypeListWithMinTypeFirst, std::tuple<>>
                ::value, "Something went wrong in the  implementation of the "
                "Sort trait");

        /**
         * The difference between the size of the current list and the size of
         * the list that has the min type in it as the first element will be
         * the location where the erase should be called.  For example if the
         * original list was <int, char, float, int> and the lowest value was
         * char, then the TypeListWithMinTypeFirst will be
         * std::tuple<char, float, int>, and then the location to call erase
         * from in the original list will be 1 which is 4 - 3.
         */
        static constexpr const int location_to_erase = sizeof...(Types)
            - std::tuple_size<TypeListWithMinTypeFirst>::value;

        /**
         * Erase the type from the tuple and then recurse
         */
        using ErasedTypeToRecurseOn = Erase_t<location_to_erase,
                                              std::tuple<Types...>>;

    public:
        using type = Concatenate_t<
            std::tuple<MinType>,
            typename SortImpl<Comparator,
                              ErasedTypeToRecurseOn>::type>;
    };

} // namespace detail

/**
 * @class AllOf
 *
 * A trait that returns true if the predicate returns true on all the types in
 * the type list, for example <std::is_reference, int&, double&> will return
 * true
 */
template <template <typename...> class Predicate, typename TypeList>
struct AllOf {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    static constexpr const bool value
        = detail::AllOfImpl<Predicate, TypeList>::value;
};

/**
 * @class AnyOf
 *
 * A trait that returns true if the predicate returns true on any of the types
 * in the type list, for example <std::is_reference, int&, double> will
 * return true
 */
template <template <typename...> class Predicate, typename TypeList>
struct AnyOf {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    static constexpr const bool value
        = detail::AnyOfImpl<Predicate, TypeList>::value;
};

/**
 * @class NoneOf
 *
 * A trait that returns a true if the predicate returns true for none of the
 * types in the type list, this is just the negation of AnyOf
 */
template <template <typename...> class Predicate, typename TypeList>
struct NoneOf {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    static constexpr const bool value = !AnyOf<Predicate, TypeList>::value;
};

/**
 * @class ForEach
 *
 * The class is a function object that when initialized iterates through a
 * given range of types by calling the functor on the type and a context type,
 * this context type is defined as an empty object that has a member typedef
 * defined (type) that is typedef-ed to the type the iteration is on, for
 * example
 *
 *      ForEach<int, double>{}([&](auto type_context) {
 *          cout << typeid(typename decltype(context)::type) << endl;
 *      });
 */
template <typename TypeList>
struct ForEach {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");

    template <typename Func>
    Func operator()(Func func) {
        detail::ForEachImpl<TypeList>{}(func);
        return func;
    }
};

/**
 * @class CountIf
 *
 * A trait that counts the number of times the predicate returns true on the
 * type list
 */
template <template <typename...> class Predicate, typename TypeList>
struct CountIf {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    static constexpr const int value
        = detail::CountIfImpl<Predicate, TypeList>::value;
};

/**
 * @class Mismatch
 *
 * A trait that returns the first types that are not the same in the two
 * provided type lists
 */
template <typename TypesContainerOne, typename TypesContainerTwo>
struct Mismatch {
    static_assert(sharp::IsInstantiationOf_v<TypesContainerOne, std::tuple> &&
            sharp::IsInstantiationOf_v<TypesContainerTwo, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type = typename detail::MismatchImpl<TypesContainerOne,
                                               TypesContainerTwo>::type;
};

/**
 * @class Equal
 *
 * A trait algorithm that helps you determine whether two ranges are equal are
 * not, this is different from std::is_same because it helps you determine
 * whether two ranges are equal even when they are in reality not the same
 * length, for example some (or one) overloads (or overload) of std::equal
 * do (or does) not check for equality of the lengths of the ranges they were
 * passed, they simply compare
 */
template <typename TypesContainerOne, typename TypesContainerTwo>
struct Equal {
    static_assert(sharp::IsInstantiationOf_v<TypesContainerOne, std::tuple> &&
            sharp::IsInstantiationOf_v<TypesContainerTwo, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    static constexpr const bool value = detail::EqualImpl<
        TypesContainerOne, TypesContainerTwo>::value;
};

/**
 * @class FindIf
 *
 * A trait that lets you find the first type for which the predicate returned
 * true, similar to std::find_if
 */
template <template <typename...> class Predicate, typename TypeList>
struct FindIf {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type
        = typename detail::FindIfImpl<Predicate, TypeList>::type;
};

/**
 * @class Find
 *
 * A trait that lets you find the type passed in, this is implemented using
 * the FindIf trait and a std::is_same predicate
 */
template <typename ToFind, typename TypeList>
struct Find {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type = typename FindIf<Bind<std::is_same, ToFind>::template type,
                                 TypeList>::type;
};

/**
 * @class FindIndex
 *
 * A trait that lets you find the location of the type passed in the type
 * list, which is provided as the second argument.  If the type is in the list
 * then the index of the first matching type in the list is returned,
 * otherwise std::tuple_size<TypeList>::value is returned
 */
template <typename ToFind, typename TypeList>
struct FindIndex {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");

    // get the result tuple from sharp::Find, and then use that to get the
    // index where the type is found in the type list
    using FindResultTuple = typename Find<ToFind, TypeList>::type;
    static constexpr const int value = std::tuple_size<TypeList>::value
        - std::tuple_size<FindResultTuple>::value;
};

/**
 * @class FindIfNot
 *
 * A trait that lets you find the first type for which the predicate returned
 * false, similar to std::find_if_not
 */
template <template <typename...> class Predicate, typename TypeList>
struct FindIfNot {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type = typename FindIf<Negate<Predicate>::template type,
                                 TypeList>::type;
};

/**
 * @class FindFirstOf
 *
 * A trait that lets you find the first type in the first list that matches
 * any element in the second list
 */
template <typename TypesContainerOne, typename TypesContainerTwo>
struct FindFirstOf {
    static_assert(sharp::IsInstantiationOf_v<TypesContainerOne, std::tuple> &&
            sharp::IsInstantiationOf_v<TypesContainerTwo, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type = typename detail::FindFirstOfImpl<TypesContainerOne,
          TypesContainerTwo>::type;
};

/**
 * @class AdjacentFind
 *
 * A trait that lets you find the first time a type was repeated in a type
 * list, it returns the repeated type, if no type was repeated, then this will
 * return the std::tuple<> type
 */
template <typename TypeList>
struct AdjacentFind {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type = typename detail::AdjacentFindImpl<TypeList>::type;
};

/**
 * @class Search
 *
 * A trait that searches for the second range in the first range.  similar to
 * std::search
 */
template <typename TypesContainerOne, typename TypesContainerTwo>
struct Search {
    static_assert(sharp::IsInstantiationOf_v<TypesContainerOne, std::tuple> &&
            sharp::IsInstantiationOf_v<TypesContainerTwo, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type = typename detail::SearchImpl<TypesContainerOne,
                                             TypesContainerTwo>::type;
};

/**
 * @class SearchN
 *
 * Trait that searches for n occurences of a given element, similar to
 * std::search_n
 */
template <typename TypeToRepeat, int n, typename TypeList>
struct SearchN {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type = typename detail::SearchNImpl<TypeToRepeat, n, TypeList>::type;
};

/**
 * @class TransformIf
 *
 * Transforms a range to another given by the transformation function, unlike
 * the regular transform function this one only transforms the elements if the
 * predicate returns a true
 */
template <template <typename...> class Predicate,
          template <typename...> class TransformFunction,
          typename TypeList>
struct TransformIf {
    using type = typename detail::TransformIfImpl<Predicate,
                                                  TransformFunction,
                                                  TypeList>::type;
};

/**
 * @class Transform
 *
 * Transforms a type range by applying the function passed to the trait and
 * returns the range wrapped in a tuple
 */
template <template <typename...> class TransformFunction, typename TypeList>
struct Transform {
private:
    /**
     * Function that returns a true for all types so that the transformation
     * function is applied on all the types
     */
    template <typename...>
    struct ReturnTrue : std::integral_constant<bool, true> {};

public:
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type
        = typename TransformIf<ReturnTrue, TransformFunction, TypeList>::type;
};

/**
 * @class RemoveIf
 *
 * Removes types given in a range that match the given predicate, for example
 * if the goal was to remove all elements that are references from a type
 * list, then one could do
 *
 * RemoveIf_t<std::is_reference, int&, double, bool> and get back
 * std::tuple<double, bool>
 */
template <template <typename...> class Predicate, typename TypeList>
struct RemoveIf {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type = typename detail::RemoveIfImpl<Predicate, TypeList>::type;
};

/**
 * @class Reverse
 *
 * Reverses a range of types
 */
template <typename TypeList>
struct Reverse {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type = typename detail::ReverseImpl<TypeList>::type;
};

/**
 * @class Unique
 *
 * Removes all duplicate types from a type list
 */
template <typename TypeList>
struct Unique {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type = typename detail::UniqueImpl<TypeList>::type;
};

/**
 * @class Sort
 *
 * Does what you think it does, similar to std::sort.  The implementation uses
 * a selection sort to sort the elements in the type list according to the
 * comparator and is therefore not stable
 */
template <template <typename...> class Comparator, typename TypeList>
struct Sort {
    static_assert(sharp::IsInstantiationOf_v<TypeList, std::tuple>,
            "Type list algorithms only work with type lists in std::tuples");
    using type
        = typename detail::SortImpl<Comparator, TypeList>::type;
};

/**
 * @class Max
 *
 * Determines the maximum of the given integral values.  If types are given
 * instead then the user must provide a comparator that can be used to compare
 * two different types and test which one is smaller, i.e. the predicate
 * should behave the same way as it does in the standard library, it should
 * be a logical less than operator, for example the type A is smaller than the
 * type B if Comparator<A, B>::value is true
 */
template <int... integers>
struct MaxValue {
    static constexpr const int value = detail::MaxValueImpl<integers...>::value;
};
template <template <typename...> class Comparator, typename... Types>
struct MaxType {
    using type = typename detail::MaxTypeImpl<Comparator, Types...>::type;
};

/**
 * @class Min
 *
 * Determines the maximum of the given integral values.  If types are given
 * instead then the user must provide a comparator that can be used to compare
 * two different types and test which one is smaller, i.e. the predicate
 * should behave the same way as it does in the standard library, it should
 * be a logical less than operator, for example the type A is smaller than the
 * type B if Comparator<A, B>::value is true
 */
template <int... integers>
struct MinValue {
    static constexpr const int value = detail::MinValueImpl<integers...>::value;
};
template <template <typename...> class Comparator, typename... Types>
struct MinType {
    using type
        = typename MaxType<Negate<Comparator>::template type, Types...>::type;
};

/**
 * Conventional value typedefs, these end in the suffix _v, this is keeping in
 * convention with the C++ standard library features post and including C++17
 */
template <template <typename...> class Predicate, typename TypeList>
constexpr const bool AllOf_v = AllOf<Predicate, TypeList>::value;
template <template <typename...> class Predicate, typename TypeList>
constexpr const bool AnyOf_v = AnyOf<Predicate, TypeList>::value;
template <template <typename...> class Predicate, typename TypeList>
constexpr const bool NoneOf_v = NoneOf<Predicate, TypeList>::value;
template <template <typename...> class Predicate, typename TypeList>
constexpr const int CountIf_v = CountIf<Predicate, TypeList>::value;
template <int... integers>
constexpr const int MaxValue_v = MaxValue<integers...>::value;
template <int... integers>
constexpr const int MinValue_v = MinValue<integers...>::value;
template <typename TypesContainerOne, typename TypesContainerTwo>
constexpr const bool Equal_v = Equal<TypesContainerOne, TypesContainerTwo>
    ::value;
template <typename ToFind, typename TypeList>
constexpr const int FindIndex_v = FindIndex<ToFind, TypeList>::value;

/**
 * Conventional typedefs, these end in the suffix _t, this is keeping in
 * convention with the C++ standard library features post and including C++17
 */
template <typename TypesContainerOne, typename TypesContainerTwo>
using Mismatch_t = typename Mismatch<TypesContainerOne, TypesContainerTwo>
    ::type;
template <template <typename...> class Predicate, typename TypeList>
using FindIf_t = typename FindIf<Predicate, TypeList>::type;
template <typename ToFind, typename TypeList>
using Find_t = typename Find<ToFind, TypeList>::type;
template <template <typename...> class Predicate, typename TypeList>
using FindIfNot_t = typename FindIfNot<Predicate, TypeList>::type;
template <typename TypesContainerOne, typename TypesContainerTwo>
using FindFirstOf_t = typename FindFirstOf<
    TypesContainerOne, TypesContainerTwo>::type;
template <typename TypeList>
using AdjacentFind_t = typename AdjacentFind<TypeList>::type;
template <template <typename...> class Comparator, typename... Types>
using MaxType_t = typename MaxType<Comparator, Types...>::type;
template <template <typename...> class Comparator, typename... Types>
using MinType_t = typename MinType<Comparator, Types...>::type;
template <typename TypesContainerOne, typename TypesContainerTwo>
using Search_t = typename Search<TypesContainerOne, TypesContainerTwo>::type;
template <typename TypeToRepeat, int n, typename TypeList>
using SearchN_t = typename SearchN<TypeToRepeat, n, TypeList>::type;
template <template <typename...> class TransformFunction, typename TypeList>
using Transform_t = typename Transform<TransformFunction, TypeList>::type;
template <template <typename...> class Predicate, typename TypeList>
using RemoveIf_t = typename RemoveIf<Predicate, TypeList>::type;
template <typename TypeList>
using Reverse_t = typename Reverse<TypeList>::type;
template <template <typename...> class Predicate,
          template <typename...> class TransformFunction,
          typename TypeList>
using TransformIf_t = typename TransformIf<Predicate, TransformFunction,
                                           TypeList>::type;
template <typename TypeList>
using Unique_t = typename Unique<TypeList>::type;
template <template <typename...> class Comparator, typename TypeList>
using Sort_t = typename Sort<Comparator, TypeList>::type;

/*******************************************************************************
 * Tests
 ******************************************************************************/
namespace detail { namespace test {
    template <typename ValueListOne, typename ValueListTwo>
    struct LessThanValueList;
    template <int value_one, int value_two>
    struct LessThanValueList<ValueList<value_one>, ValueList<value_two>> {
        static constexpr const bool value = value_one < value_two;
    };
    template <typename One, typename Two>
    struct LessThanSize {
        static constexpr const bool value = sizeof(One) < sizeof(Two);
    };
}}

/**
 * Tests for AllOf
 */
static_assert(AllOf_v<std::is_reference, std::tuple<>>,
        "sharp::AllOf tests failed!");
static_assert(AllOf_v<std::is_reference, std::tuple<int&>>,
        "sharp::AllOf tests failed!");
static_assert(!AllOf_v<std::is_reference, std::tuple<int&, double>>,
        "sharp::AllOf tests failed!");
static_assert(!AllOf_v<std::is_reference, std::tuple<int, double&>>,
        "sharp::AllOf tests failed!");
static_assert(AllOf_v<std::is_reference, std::tuple<int&, double&>>,
        "sharp::AllOf tests failed!");

/**
 * Tests for AnyOf
 */
static_assert(!AnyOf_v<std::is_reference, std::tuple<>>,
        "sharp::AnyOf tests failed!");
static_assert(AnyOf_v<std::is_reference, std::tuple<int&>>,
        "sharp::AnyOf tests failed!");
static_assert(AnyOf_v<std::is_reference, std::tuple<int&, double>>,
        "sharp::AnyOf tests failed!");
static_assert(AnyOf_v<std::is_reference, std::tuple<int, double&>>,
        "sharp::AnyOf tests failed!");
static_assert(!AnyOf_v<std::is_reference, std::tuple<int*, double*>>,
        "sharp::AnyOf tests failed!");
static_assert(AnyOf_v<std::is_reference, std::tuple<int&, double&>>,
        "sharp::AnyOf tests failed!");

/**
 * Tests for NoneOf
 */
static_assert(NoneOf_v<std::is_reference, std::tuple<>>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, std::tuple<int&>>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, std::tuple<int&, double>>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, std::tuple<int, double&>>,
        "sharp::NoneOf tests failed!");
static_assert(NoneOf_v<std::is_reference, std::tuple<int*, double*>>,
        "sharp::NoneOf tests failed!");
static_assert(!NoneOf_v<std::is_reference, std::tuple<int&, double&>>,
        "sharp::NoneOf tests failed!");

/**
 * Tests for CountIf
 */
static_assert(CountIf_v<std::is_reference, std::tuple<>> == 0,
        "sharp::CountIf tests failed!");
static_assert(CountIf_v<std::is_reference, std::tuple<int&>> == 1,
        "sharp::CountIf tests failed!");
static_assert(CountIf_v<std::is_reference, std::tuple<int&, double>> == 1,
        "sharp::CountIf tests failed!");
static_assert(CountIf_v<std::is_reference, std::tuple<int&, double&>> == 2,
        "sharp::CountIf tests failed!");

/**
 * Tests for Max
 */
static_assert(MaxValue_v<> == -1, "sharp::Max tests failed!");
static_assert(MaxValue_v<1> == 1, "sharp::Max tests failed!");
static_assert(MaxValue_v<1, 2> == 2, "sharp::Max tests failed!");
static_assert(MaxValue_v<1, 2, 3> == 3, "sharp::Max tests failed!");
static_assert(MaxValue_v<-1, 2, 3> == 3, "sharp::Max tests failed!");

static_assert(std::is_same<MaxType_t<detail::test::LessThanValueList,
                                     ValueList<0>, ValueList<1>>,
                           ValueList<1>>::value, "sharp::Max tests failed!");
static_assert(std::is_same<MaxType_t<detail::test::LessThanValueList,
                                     ValueList<1>, ValueList<0>>,
                           ValueList<1>>::value, "sharp::Max tests failed!");

/**
 * Tests for Min
 */
static_assert(MinValue_v<> == -1, "sharp::Min tests failed!");
static_assert(MinValue_v<1> == 1, "sharp::Min tests failed!");
static_assert(MinValue_v<1, 2> == 1, "sharp::Min tests failed!");
static_assert(MinValue_v<1, 2, 3> == 1, "sharp::Min tests failed!");
static_assert(MinValue_v<-1, 2, 3> == -1, "sharp::Min tests failed!");
static_assert(std::is_same<MinType_t<detail::test::LessThanValueList,
                                     ValueList<0>, ValueList<1>>,
                           ValueList<0>>::value, "sharp::Min tests failed!");
static_assert(std::is_same<MinType_t<detail::test::LessThanValueList,
                                     ValueList<1>, ValueList<0>>,
                           ValueList<0>>::value, "sharp::Min tests failed!");

/**
 * Tests for Mismatch
 */
static_assert(std::is_same<Mismatch_t<std::tuple<int, double, char>,
                                      std::tuple<int, double, char*>>,
                           std::pair<std::tuple<char>, std::tuple<char*>>>
        ::value, "sharp::Mismatch tests failed!");
static_assert(std::is_same<Mismatch_t<std::tuple<>, std::tuple<>>,
                           std::pair<std::tuple<>, std::tuple<>>>::value,
        "sharp::Mismatch tests failed!");
static_assert(std::is_same<Mismatch_t<std::tuple<int, double>, std::tuple<>>,
                           std::pair<std::tuple<int, double>, std::tuple<>>>
        ::value, "sharp::Mismatch tests failed!");
static_assert(std::is_same<Mismatch_t<std::tuple<>, std::tuple<int, double>>,
                           std::pair<std::tuple<>, std::tuple<int, double>>>
        ::value, "sharp::Mismatch tests failed!");
static_assert(std::is_same<Mismatch_t<std::tuple<int, char*>,
                                      std::tuple<int, double>>,
                           std::pair<std::tuple<char*>, std::tuple<double>>>
        ::value, "sharp::Mismatch tests failed!");
static_assert(std::is_same<Mismatch_t<std::tuple<int, char*>,
                                      std::tuple<int, double, bool, char>>,
                           std::pair<std::tuple<char*>,
                                     std::tuple<double, bool, char>>>
        ::value, "sharp::Mismatch tests failed!");

/**
 * Tests for Equal
 */
static_assert(Equal_v<std::tuple<int, double>, std::tuple<int, double>>,
        "sharp::Equal tests failed!");
static_assert(Equal_v<std::tuple<>, std::tuple<>>,
        "sharp::Equal tests failed!");
static_assert(Equal_v<std::tuple<int, double>, std::tuple<int, double, int>>,
        "sharp::Equal tests failed!");
static_assert(!Equal_v<std::tuple<int, double, char>, std::tuple<int, double>>,
        "sharp::Equal tests failed!");
static_assert(!Equal_v<std::tuple<double, char>, std::tuple<int, double>>,
        "sharp::Equal tests failed!");
static_assert(!Equal_v<std::tuple<int, double, char>, std::tuple<double>>,
        "sharp::Equal tests failed!");

/**
 * Tests for FindIf
 */
static_assert(std::is_same<FindIf_t<std::is_reference, std::tuple<>>,
                           std::tuple<>>::value,
        "sharp::FindIf tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference, std::tuple<int, int&>>,
        std::tuple<int&>>::value, "sharp::FindIf tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference, std::tuple<int*, int&>>,
        std::tuple<int&>>::value, "sharp::FindIf tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference, std::tuple<double, int>>,
                           std::tuple<>>::value, "sharp::FindIf tests failed!");
static_assert(std::is_same<FindIf_t<std::is_reference,
                                    std::tuple<double&, int>>,
        std::tuple<double&, int>>::value, "sharp::FindIf tests failed!");

/**
 * Tests for Find
 */
static_assert(std::is_same<Find_t<int, std::tuple<>>, std::tuple<>>::value,
        "sharp::Find tests failed!");
static_assert(std::is_same<Find_t<int, std::tuple<double, int>>,
                           std::tuple<int>>::value,
        "sharp::Find tests failed!");
static_assert(std::is_same<Find_t<int, std::tuple<int, double>>,
        std::tuple<int, double>>::value, "sharp::FindIf tests failed!");
static_assert(std::is_same<Find_t<int, std::tuple<double*, int>>,
                           std::tuple<int>>::value,
        "sharp::Find tests failed!");
static_assert(std::is_same<Find_t<int, std::tuple<double*, int, bool>>,
                           std::tuple<int, bool>>::value,
        "sharp::Find tests failed!");

static_assert(FindIndex_v<int, std::tuple<>> == 0,
        "sharp::FindIndex tests failed!");
static_assert(FindIndex_v<int, std::tuple<double, int>> == 1,
        "sharp::FindIndex tests failed!");
static_assert(FindIndex_v<int, std::tuple<int, double>> == 0,
        "sharp::FindIf tests failed!");
static_assert(FindIndex_v<int, std::tuple<double*, int>> == 1,
        "sharp::FindIndex tests failed!");
static_assert(FindIndex_v<int, std::tuple<double*, int, bool>> == 1,
        "sharp::FindIndex tests failed!");

/**
 * Tests for FindIfNot
 */
static_assert(std::is_same<FindIfNot_t<std::is_reference, std::tuple<>>,
                           std::tuple<>>::value,
        "sharp::FindIfNot tests failed!");
static_assert(std::is_same<FindIfNot_t<std::is_reference,
                                       std::tuple<int, int&>>,
        std::tuple<int, int&>>::value, "sharp::FindItNot tests failed!");
static_assert(std::is_same<FindIfNot_t<std::is_reference,
                                       std::tuple<int*, int&>>,
        std::tuple<int*, int&>>::value, "sharp::FindItNot tests failed!");
static_assert(std::is_same<FindIfNot_t<std::is_reference,
                                       std::tuple<int&, double, int>>,
        std::tuple<double, int>>::value, "sharp::FindIfNot tests failed!");

/**
 * Tests for FindFirstOf
 */
static_assert(std::is_same<FindFirstOf_t<std::tuple<>, std::tuple<>>,
                           std::tuple<>>::value,
        "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int>, std::tuple<>>,
                           std::tuple<>>::value,
        "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<>, std::tuple<int>>,
                           std::tuple<>>::value,
       "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int, double>,
                                         std::tuple<>>, std::tuple<>>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<>,
                                         std::tuple<int, double>>, std::tuple<>>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int, double>,
                                         std::tuple<char, double>>,
                                         std::tuple<double>>
        ::value, "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int, double*>,
                                         std::tuple<char, double>>,
                           std::tuple<>>::value,
       "sharp::FindFirstOf tests failed!");
static_assert(std::is_same<FindFirstOf_t<std::tuple<int, double*>,
                                         std::tuple<int, double>>,
                                         std::tuple<int, double*>>
        ::value, "sharp::FindFirstOf tests failed!");

/**
 * Tests for AdjacentFind
 */
static_assert(std::is_same<AdjacentFind_t<std::tuple<int, double, char>>,
                           std::tuple<>>
        ::value, "sharp::AdjacentFind tests failed!");
static_assert(std::is_same<AdjacentFind_t<std::tuple<int, int, char>>,
        std::tuple<int, int, char>>::value,
        "sharp::AdjacentFind tests failed!");
static_assert(std::is_same<AdjacentFind_t<std::tuple<char, int, int>>,
        std::tuple<int, int>>::value, "sharp::AdjacentFind tests failed!");
static_assert(std::is_same<AdjacentFind_t<std::tuple<int*, double&, int*>>,
                           std::tuple<>>
        ::value, "sharp::AdjacentFind tests failed!");

/**
 * Tests for Search
 */
static_assert(std::is_same<Search_t<std::tuple<double, char>,
                                    std::tuple<double, char>>,
                           std::tuple<double, char>>::value,
        "sharp::Search tests failed!");
static_assert(std::is_same<Search_t<std::tuple<int, double, char>,
                                    std::tuple<double, char>>,
                           std::tuple<double, char>>::value,
        "sharp::Search tests failed!");
static_assert(std::is_same<Search_t<std::tuple<int, double, int, char>,
                                    std::tuple<double, int>>,
                           std::tuple<double, int, char>>::value,
        "sharp::Search tests failed!");
static_assert(std::is_same<Search_t<std::tuple<double>,
                                    std::tuple<double, int>>,
                           std::tuple<>>::value,
        "sharp::Search tests failed!");
static_assert(std::is_same<Search_t<std::tuple<int, double, char>,
                                    std::tuple<int>>,
                           std::tuple<int, double, char>>::value,
        "sharp::Search tests failed!");
static_assert(std::is_same<Search_t<std::tuple<int, double, char>,
                                    std::tuple<double>>,
                           std::tuple<double, char>>::value,
        "sharp::Search tests failed!");
static_assert(std::is_same<Search_t<std::tuple<int, double, char>,
                                    std::tuple<>>,
                           std::tuple<int, double, char>>::value,
        "sharp::Search tests failed!");
static_assert(std::is_same<Search_t<std::tuple<int, double, char, float>,
                                    std::tuple<double, char>>,
                           std::tuple<double, char, float>>::value,
        "sharp::Search tests failed!");
static_assert(std::is_same<Search_t<std::tuple<>,
                                    std::tuple<>>,
                           std::tuple<>>::value,
        "sharp::Search tests failed!");
static_assert(std::is_same<Search_t<std::tuple<double, int, int, int, double>,
                                    std::tuple<int, int, int>>,
                           std::tuple<int, int, int, double>>::value,
        "sharp::Search tests failed!");


/**
 * Tests for SearchN
 */
static_assert(std::is_same<SearchN_t<int, 3,
                                     std::tuple<double, int, int, int, double>>,
                           std::tuple<int, int, int, double>>::value,
        "sharp::SearchN tests failed");
static_assert(std::is_same<SearchN_t<int, 1,
                                     std::tuple<double, int, int, int, double>>,
                           std::tuple<int, int, int, double>>::value,
        "sharp::SearchN tests failed");
static_assert(std::is_same<SearchN_t<
                                 double, 2,
                                 std::tuple<double, int, double, double, int>>,
                           std::tuple<double, double, int>>::value,
        "sharp::SearchN tests failed");
static_assert(std::is_same<SearchN_t<double, 3,
                                     std::tuple<double, int, int, int, double>>,
                           std::tuple<>>::value,
        "sharp::SearchN tests failed");

/**
 * Tests for TransformIf
 */
static_assert(std::is_same<TransformIf_t<std::is_reference,
                                         std::remove_reference,
                                         std::tuple<int, double, int&, char>>,
                           std::tuple<int, double, int, char>>::value,
        "sharp::TransformIf tests failed");
static_assert(std::is_same<TransformIf_t<std::is_reference,
                                         std::remove_reference,
                                         std::tuple<int*, double&, int&, char>>,
                           std::tuple<int*, double, int, char>>::value,
        "sharp::TransformIf tests failed");
static_assert(std::is_same<TransformIf_t<std::is_reference,
                                         std::remove_reference,
                                         std::tuple<int>>,
                           std::tuple<int>>::value,
        "sharp::TransformIf tests failed");

/**
 * Tests for Transform
 */
static_assert(std::is_same<Transform_t<std::remove_reference,
                                       std::tuple<int&, double&>>,
                           std::tuple<int, double>>::value,
        "sharp::Transform tests failed");
static_assert(std::is_same<
        Transform_t<std::remove_pointer,
                    std::tuple<std::add_pointer_t<int&>, double&>>,
        std::tuple<int, double&>>::value,
        "sharp::Transform tests failed");
static_assert(std::is_same<Transform_t<std::remove_reference, std::tuple<>>,
                           std::tuple<>>::value,
        "sharp::Transform tests failed");
static_assert(std::is_same<Transform_t<std::decay,
                                       std::tuple<const int&, volatile char&>>,
                           std::tuple<int, char>>::value,
        "sharp::Transform tests failed");

/**
 * Tests for RemoveIf
 */
static_assert(std::is_same<RemoveIf_t<std::is_reference,
                                      std::tuple<int, int&, char>>,
                           std::tuple<int, char>>::value,
        "sharp::RemoveIf tests failed");
static_assert(std::is_same<RemoveIf_t<std::is_pointer,
                                      std::tuple<int*, int*, char>>,
                           std::tuple<char>>::value,
        "sharp::RemoveIf tests failed");
static_assert(std::is_same<RemoveIf_t<std::is_pointer,
                                      std::tuple<int, int, char>>,
                           std::tuple<int, int, char>>::value,
        "sharp::RemoveIf tests failed");
static_assert(std::is_same<RemoveIf_t<std::is_pointer, std::tuple<int*>>,
                           std::tuple<>>::value,
        "sharp::RemoveIf tests failed");
static_assert(std::is_same<RemoveIf_t<std::is_pointer, std::tuple<int>>,
                           std::tuple<int>>::value,
        "sharp::RemoveIf tests failed");
static_assert(std::is_same<RemoveIf_t<std::is_pointer, std::tuple<>>,
                           std::tuple<>>::value,
        "sharp::RemoveIf tests failed");

/**
 * Tests for reverse
 */
static_assert(std::is_same<Reverse_t<std::tuple<int, char>>,
                           std::tuple<char, int>>::value,
        "sharp::Reverse tests failed!");
static_assert(std::is_same<Reverse_t<std::tuple<>>, std::tuple<>>::value,
        "sharp::Reverse tests failed!");
static_assert(std::is_same<Reverse_t<std::tuple<int, char, bool, double>>,
                           std::tuple<double, bool, char, int>>::value,
        "sharp::Reverse tests failed!");
static_assert(std::is_same<Reverse_t<std::tuple<int>>, std::tuple<int>>::value,
        "sharp::Reverse tests failed!");
static_assert(std::is_same<Reverse_t<std::tuple<char, int>>,
                           std::tuple<int, char>>::value,
        "sharp::Reverse tests failed!");

/**
 * Tests for Unique
 */
static_assert(std::is_same<Unique_t<std::tuple<int, double, int>>,
                           std::tuple<int, double>>
        ::value, "sharp::Unique tests failed");
static_assert(std::is_same<Unique_t<std::tuple<int, int, double>>,
                           std::tuple<int, double>>
        ::value, "sharp::Unique tests failed");
static_assert(std::is_same<Unique_t<std::tuple<double, int, int>>,
                           std::tuple<double, int>>
        ::value, "sharp::Unique tests failed");

/**
 * Tests for Sort
 */
static_assert(std::is_same<Sort_t<detail::test::LessThanSize,
                                  std::tuple<std::uint32_t, std::uint16_t,
                                             std::uint8_t>>,
                           std::tuple<std::uint8_t, std::uint16_t,
                                      std::uint32_t>>::value,
        "sharp::Sort tests failed");
static_assert(std::is_same<Sort_t<detail::test::LessThanSize,
                                  std::tuple<std::uint8_t, std::uint16_t,
                                             std::uint32_t>>,
                           std::tuple<std::uint8_t, std::uint16_t,
                                      std::uint32_t>>::value,
        "sharp::Sort tests failed");
static_assert(std::is_same<Sort_t<detail::test::LessThanSize,
                                  std::tuple<std::uint16_t, std::uint8_t,
                                             std::uint32_t>>,
                           std::tuple<std::uint8_t, std::uint16_t,
                                      std::uint32_t>>::value,
        "sharp::Sort tests failed");
static_assert(std::is_same<Sort_t<detail::test::LessThanSize,
                                  std::tuple<std::uint16_t, std::uint8_t,
                                             std::uint32_t>>,
                           std::tuple<std::uint8_t, std::uint16_t,
                                      std::uint32_t>>::value,
        "sharp::Sort tests failed");
static_assert(std::is_same<Sort_t<detail::test::LessThanSize,
                                  std::tuple<std::uint16_t, std::uint32_t,
                                             std::uint8_t>>,
                           std::tuple<std::uint8_t, std::uint16_t,
                                      std::uint32_t>>::value,
        "sharp::Sort tests failed");
static_assert(std::is_same<Sort_t<detail::test::LessThanSize,
                                  std::tuple<std::uint8_t, std::uint32_t,
                                             std::uint16_t>>,
                           std::tuple<std::uint8_t, std::uint16_t,
                                      std::uint32_t>>::value,
        "sharp::Sort tests failed");

} // namespace sharp
