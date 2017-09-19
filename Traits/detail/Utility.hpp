/**
 * @file Utility.hpp
 * @author Aaryaman Sagar
 *
 * Functions and traits that do not fit in any of the files in the module
 * above, so they are dumped here
 */

#pragma once

#include <type_traits>
#include <tuple>

namespace sharp {

/**
 * @class ValueList
 *
 * A constexpr value container, this can be used to store values at compile
 * time.  At the moment before C++17 this only stores integer values
 */
template <int... values>
struct ValueList {};

namespace detail {

    /**
     * Implemenation for the concatenate trait
     */
    template <typename... Lists>
    struct ConcatenateImpl;
    template <template <typename...> class Container,
              typename... TypesOne, typename... TypesTwo,
              typename... Lists>
    struct ConcatenateImpl<Container<TypesOne...>, Container<TypesTwo...>,
                           Lists...> {
        using type = typename ConcatenateImpl<
            Container<TypesOne..., TypesTwo...>,
            Lists...>::type;
    };
    template <template <typename...> class Container,
              typename... TypesOne, typename... TypesTwo>
    struct ConcatenateImpl<Container<TypesOne...>, Container<TypesTwo...>> {
        using type = Container<TypesOne..., TypesTwo...>;
    };
    template <int... integers_one, int... integers_two>
    struct ConcatenateImpl<ValueList<integers_one...>,
                           ValueList<integers_two...>> {
        using type = ValueList<integers_one..., integers_two...>;
    };
    template <int... integers_one, int... integers_two, typename... Lists>
    struct ConcatenateImpl<ValueList<integers_one...>,
                           ValueList<integers_two...>,
                           Lists...> {
        using type = typename ConcatenateImpl<
            ValueList<integers_one..., integers_two...>,
            Lists...>::type;
    };

    /**
     * Implementation for the PopFront trait
     */
    template <typename TypesContainer>
    struct PopFrontImpl;
    template <template <typename...> class Container,
              typename Head, typename... Tail>
    struct PopFrontImpl<Container<Head, Tail...>> {
        using type = Container<Tail...>;
    };
    template <template <typename...> class Container>
    struct PopFrontImpl<Container<>> {
        using type = Container<>;
    };

    /**
     * Implementation for the erase trait
     */
    template <int to_erase, typename TypesContainer>
    struct EraseImpl;
    template <int to_erase, template <typename...> class Container,
              typename... Types>
    struct EraseImpl<to_erase, Container<Types...>> {
        using type = Container<Types...>;
    };
    template <int to_erase, template <typename...> class Container,
              typename Head, typename... Types>
    struct EraseImpl<to_erase, Container<Head, Types...>> {

        static_assert(to_erase > 0, "Something went wrong in the "
                "implementation of the Erase trait");

        using type = typename ConcatenateImpl<
            Container<Head>,
            typename EraseImpl<to_erase - 1, Container<Types...>>::type>::type;
    };
    template <template <typename...> class Container,
              typename Head, typename... Types>
    struct EraseImpl<0, Container<Head, Types...>> {
        using type = Container<Types...>;
    };

    /**
     * Implemenation for the ConcatenateN trait
     */
    template <typename TypeToRepeat, int n>
    struct ConcatenateNImpl {
        using type = typename ConcatenateImpl<
            std::tuple<TypeToRepeat>,
            typename ConcatenateNImpl<TypeToRepeat, n - 1>::type>::type;
    };
    template <typename TypeToRepeat>
    struct ConcatenateNImpl<TypeToRepeat, 0> {
        using type = std::tuple<>;
    };

    /**
     * Implementation for the MatchReference trait
     */
    template <typename TypeToMatch, typename Type>
    struct MatchReferenceImpl;
    template <typename TypeToMatch, typename Type>
    struct MatchReferenceImpl<TypeToMatch&, Type> {
        using type = Type&;
    };
    template <typename TypeToMatch, typename Type>
    struct MatchReferenceImpl<const TypeToMatch&, Type> {
        using type = const Type&;
    };
    template <typename TypeToMatch, typename Type>
    struct MatchReferenceImpl<TypeToMatch&&, Type> {
        using type = Type&&;
    };
    template <typename TypeToMatch, typename Type>
    struct MatchReferenceImpl<const TypeToMatch&&, Type> {
        using type = const Type&&;
    };

} // namespace detail

/**
 * @class Concatenate
 *
 * concatenates two type lists or two value lists, type lists are supported as
 * std::tuples and value lists are supported as sharp::ValueList
 */
template <typename... Lists>
struct Concatenate {
    using type = typename detail::ConcatenateImpl<Lists...>::type;
};

/**
 * @class ConcatenateN
 *
 * Concatenates the same type n types to result in a tuple of length n with
 * the same type repeated n times
 */
template <typename TypeToRepeat, int n>
struct ConcatenateN {
    using type = typename detail::ConcatenateNImpl<TypeToRepeat, n>::type;
};

/**
 * @class PopFront
 *
 * Pops the first type out of the type list container and returns the rest of
 * the type container
 */
template <typename TypesContainer>
struct PopFront {
    using type = typename detail::PopFrontImpl<TypesContainer>::type;
};

/**
 * @class Erase
 *
 * Erases the element at the given index from the type list and returns the
 * type list wrapped in a std::tuple
 */
template <int to_erase, typename TypesContainer>
struct Erase {
    using type = typename detail::EraseImpl<to_erase, TypesContainer>::type;
};

/**
 * @class Identity
 *
 * An empty class with nothing but a member typedef, this is used to simplify
 * passing type parameters around to lambdas, objects of this type can be
 * passed to polymorphic lambdas as parameters and that can be used to deduce
 * the types passed to the lambda
 */
template <typename Type>
struct Identity {
    using type = Type;
};

/**
 * @class MatchReference
 *
 * This class can be used to match the reference-ness of a forwarding
 * reference on another type.  For example if the tuple is && and you want to
 * return the tuple's member, you would want the && to match in std::get()'s
 * return value, as a result, this has slightly different usage semantics as
 * compared to std::forward.  For safety.  You should only call this with
 * with the full type of the forwarding reference, and include the two &&
 */
template <typename TypeToMatch, typename Type>
struct MatchReference {
    using type = typename detail::MatchReferenceImpl<TypeToMatch, Type>::type;
};

/**
 * Conventional typedefs, these end in the suffix _t, this is keeping in
 * convention with the C++ standard library features post and including C++17
 */
template <typename... Lists>
using Concatenate_t = typename Concatenate<Lists...>::type;
template <typename TypeToRepeat, int n>
using ConcatenateN_t = typename ConcatenateN<TypeToRepeat, n>::type;
template <typename TypesContainer>
using PopFront_t = typename PopFront<TypesContainer>::type;
template <int to_erase, typename... Types>
using Erase_t = typename Erase<to_erase, Types...>::type;
template <typename TypeToMatch, typename Type>
using MatchReference_t = typename MatchReference<TypeToMatch, Type>::type;

} // namespace sharp
