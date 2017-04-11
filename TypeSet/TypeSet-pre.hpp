/**
 * @file TypeSet-pre.hpp
 * @author Aaryaman Sagar
 *
 * This file contains private things that should ideally not be a part of the
 * public header file.  This includes template metafunctions concepts that are
 * used in the public header file
 */

#pragma once

namespace sharp {

/**
 * Forward declaration of the TypeSet class.  This can then be used in the
 * private template metafunctions
 */
template <typename... Types>
class TypeSet;

namespace detail {

    /**
     * Template higher order function that checks if the second type is
     * contained in the type list from the first type
     */
    template <typename TypeList, typename Type>
    struct IsContainedIn {
        static constexpr const bool value = !std::is_same<
            sharp::Find_t<Type, TypeList>, std::tuple<>>::value;
    };

    /**
     * Template higher order function to transform a type into its
     * corresponding untyped storage
     */
    template <typename Type>
    struct AlignedStorageFor {
        using type = std::aligned_storage_t<sizeof(Type), alignof(Type)>;
    };

    /**
     * Template higher order comparator that does a less than comparison of
     * the size of two types
     */
    template <typename One, typename Two>
    struct LessThanTypes {
        static constexpr const bool value = sizeof(One) < sizeof(Two);
    };

    /**
     * Template meta function to check and make sure that the type is nothrow
     * move constructible, nothrow default constructible and nothrow copy
     * constructible
     */
    template <typename Type>
    struct IsNothrowConstructibleAllWays {
        static constexpr const bool value =
            std::is_nothrow_default_constructible<Type>::value
            && std::is_nothrow_move_constructible<Type>::value
            && std::is_nothrow_copy_constructible<Type>::value;
    };

    /**
     * Assertions documenting the invariants of the type list that can be used
     * to instantiate a type list.
     *
     * This checks to make sure tha tthe type list does not contain any
     * duplicate types and also makes sure ha tthe type list does not contain
     * any references
     */
    template <typename... Types>
    constexpr bool check_type_list_invariants = std::is_same<
        sharp::Unique_t<std::tuple<Types...>>,
        std::tuple<Types...>>::value
            && !sharp::AnyOf_v<std::is_reference, std::tuple<Types...>>
            && !sharp::AnyOf_v<std::is_const, std::tuple<Types...>>;;

    /**
     * Concepts
     */
    /**
     * Concept that checks to make sure that the other type is a type set type
     * and has the same elements as the first type set
     */
    template <typename TypeSetOne, typename TypeSetTwo>
    using EnableIfAreTypeSets = std::enable_if_t<
        sharp::IsInstantiationOf_v<TypeSetOne, sharp::TypeSet>
        && sharp::IsInstantiationOf_v<TypeSetTwo, sharp::TypeSet>
        && std::tuple_size<typename TypeSetOne::types>::value
            == std::tuple_size<typename TypeSetTwo::types>::value
        && sharp::AllOf_v<
            sharp::Bind<IsContainedIn,
                        typename TypeSetOne::types>::template type,
            typename TypeSetTwo::types>>;

    /**
     * Concept that checks whether the type is an instantiation of TypeSet,
     * this is used to disable the forwarding reference constructor from
     * overload resolution when the type passed is a TypeSet
     */
    template <typename TypeOne, typename TypeTwo>
    using EnableIfArentSame = std::enable_if_t<!std::is_same<TypeOne, TypeTwo>
        ::value>;

} // namespace detail

} // namespace sharp
