/**
 * @file TypeSet.hpp
 * @author Aaryaman Sagar
 *
 * This file contains a type set implementation.  The key differences from
 * std::tuple is that this container is optimized for space and also offers
 * some additional functionality like being able to collect a subset of type
 * instances into one type set object with a possibly larger set of types
 */

#pragma once

#include <tuple>
#include <type_traits>

#include <sharp/Traits/Traits.hpp>

namespace sharp {

namespace detail {

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
     * Assertions documenting the invariants of the type list that can be used
     * to instantiate a type list.
     *
     * This checks to make sure tha tthe type list does not contain any
     * duplicate types and also makes sure ha tthe type list does not contain
     * any references
     */
    template <typename... Types>
    constexpr bool type_list_check = std::is_same<
        sharp::Unique_t<std::tuple<Types...>>,
        std::tuple<Types...>>::value
            && !sharp::AnyOf_v<std::is_reference, std::tuple<Types...>>;

} // namespace detail

/**
 * Forward declaration for the TypeSet class
 */
template <typename... Types>
class TypeSet {
public:

    /**
     * Assert that the type list passed does not have any duplicate types,
     * that would be a violation of the invariant set by this class and would
     * cause the implementation to break
     */
    static_assert(detail::type_list_check<Types...>, "Type list malformed");

    /**
     * Default constructs the arguments provided to the type set into the
     * aligned storage, one by one
     */
    TypeSet();

    /**
     * Move constructor and copy constructor copy the type set into the other
     * type set
     */
    TypeSet(const TypeSet&);
    TypeSet(TypeSet&&) noexcept(sharp::AllOf_v<
            std::is_nothrow_move_constructible, std::tuple<Types...>>);

    /**
     * Move and copy assignment operators
     */
    TypeSet& operator=(const TypeSet&);
    TypeSet& operator=(TypeSet&&);

    /**
     * Destroys all the objects stored in the type set
     */
    ~TypeSet();

    /**
     * Query the type set to see if a type is in the type set or not
     */
    template <typename Type>
    static constexpr bool exists();

    /**
     * Make friends with all the getter functions
     *
     * TODO MatchReference_t here because of CLANG BUG, I would use
     * decltype(auto) but it does not seem to work, try it goo.gl/kNmJIr
     */
    template <typename Type, typename TypeSetType>
    friend sharp::MatchReference_t<TypeSetType&&, Type> get(TypeSetType&&);
    template <typename... OtherTypes, typename... Args>
    friend TypeSet<OtherTypes...> collect_args(Args&&... args);

private:

    /**
     * Constructs a type set that does not have any element within it
     * constructed
     */
    TypeSet(sharp::empty::tag_t);

    /**
     * The data item that is of type std::tuple,
     *
     * The Sort_t algorithm sorts the data types in the order that they should
     * appear so as to take the least amount of space if placed contiguously in
     * memory.  Reference sharp/Traits/detail/Algorithm.hpp for implementation
     * details
     *
     * The Transform_t algorithm iterates through the type list provided as an
     * argument and transforms it into a type list that contains the results
     * of applying the transformation higher order function to each type in
     * the input type list.  The result is returned as a std::tuple, reference
     * sharp/Traits/detail/Algorithm.hpp
     */
    using SortedList = Sort_t<detail::LessThanTypes, std::tuple<Types...>>;
    using TransformedList = Transform_t<detail::AlignedStorageFor, SortedList>;
    static_assert(IsInstantiationOf_v<TransformedList, std::tuple>,
            "sharp::Transform_t returned a non std::tuple type list");
    TransformedList aligned_tuple;
};

/**
 * @function get
 *
 * returns the instance of the given type from the TypeSet, this function is
 * similar in functionality to std::get<> that operates on a std::tuple and
 * returns a type from that tuple
 *
 * This function only participates in overload resolution if the type passed
 * to the function is an instantiation of type TypeSet
 *
 * The three overloads deal with const and movability with TypeSet instances
 *
 * The reason these are provided as non member functions is the same as the
 * reason the C++ standard library provides non member get<> functions for
 * std::tuple - If these were member functions, and code depended on a
 * template parameter, the member function would have to be prepended with the
 * "template" keyword.  Which is a nuisance.
 */
template <typename Type, typename TypeSetType>
sharp::MatchReference_t<TypeSetType&&, Type> get(TypeSetType&& type_set);

/**
 * @function collect_types
 *
 * Collects the arguments given into the type list provided to the function,
 * if there are missing arguments in the type list provided as arguments they
 * are default constructed into the type set.  The others are moved into the
 * type set
 *
 * A consequence of the above description is that the objects used with the
 * type set must be move constructible and are required to be default
 * constructible if the user does not intend to pass in an argument of that
 * type to the function
 */
template <typename... Types, typename... Args>
TypeSet<Types...> collect_args(Args&&... args);

} // namespace sharp

#include <sharp/TypeSet/TypeSet.ipp>
