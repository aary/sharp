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

#include <sharp/TypeSet/detail/TypeSet-pre.hpp>

namespace sharp {

/**
 * @class TypeSet
 *
 * A tuple-like class that acts as a container for instances of a set of given
 * types
 *
 *  auto type_set = TypeSet<int, double, char>{};
 *  cout << sharp::get<int>(type_set) << endl;
 *
 * Why not just use a std::tuple?  This allows for more fine control over the
 * lifetimes of the instances that are contained in the type set.  If a
 * similar utility were built around a strongly typed std::tuple, then it
 * would definitely either involve more constructions than the following
 * offers or it would eventually degrade to a loosely typed tuple
 * implementation like this class contains
 *
 * The reason this was built in the first place was to allow features like
 * named argument passing to functions.  For example if you have the following
 * function
 *
 *  template <typename... Args>
 *  void start_server(Args&&... args);
 *
 * You might be able to call it like so
 *
 *  start_server(Port{80}, debug{true});
 *  start_server(debug{true}, Port{8000});
 *  start_server(Port{80});
 *
 * All you would need to do in the implementation of start_server is to call a
 * factory function that returns an instance of a type set like so
 *
 *  template <typename... Args>
 *  void start_server(Args&&... args) {
 *      auto args = collect_args<Port, Debug>(std::forward<Args>(args)...)
 *      cout << sharp::get<Port>(args).get_port_number() << endl;
 *      cout << sharp::get<Debug>(args).is_debug << endl;
 *  }
 *
 * The effort in this class' interface was to create a type like structure
 * that would resemble an aggregate type as much as possible in its value
 * semantics.  Most of the functions that access or modify the instances of
 * this class have been written so that they can forward the value category
 * that the type set instance occurred in to the member accesses.  For example
 *
 *  sharp::get<Something>(TypeSet<Something>{})
 *
 * will return an rvalue
 *
 * All of the template metaprogramming used in this class is present in either
 * the private modules of the class or in the sharp/Traits module
 */
template <typename... Types>
class TypeSet {
public:

    /**
     * Assert that the type list passed does not have any duplicate types,
     * that would be a violation of the invariant set by this class and would
     * cause the implementation to break
     */
    static_assert(detail::check_type_list_invariants<Types...>,
            "Type list malformed");

    /**
     * Traits
     */
    using types = std::tuple<Types...>;

    /**
     * Default constructs the arguments provided to the type set into the
     * aligned storage, one by one.
     *
     * The order of construction is determined by the order in which the types
     * appear in the type list passed to the template arguments for this class
     */
    TypeSet();

    /**
     * The copy constructor for the class just copy constructs each element
     * from the other type set over into the current one.  The copy
     * construction is done in the order the types appear in the type list
     * that this class was instantiated with
     */
    TypeSet(const TypeSet&);

    /**
     * The move constructor for this class move constructs each element from
     * the other type set into the current one.  The move construction is done
     * in the order that the type appear in the type list that this class was
     * instantiated with
     *
     * The move constructor is marked conditionally noexcept when all the move
     * constructors of the instances that are present in the type set have
     * move constructors that are marked noexcept
     */
    TypeSet(TypeSet&&) noexcept(sharp::AllOf_v<
            std::is_nothrow_move_constructible, std::tuple<Types...>>);

    /**
     * The forwarding reference constructor either resolves to a move or a
     * copy constructor based on the value category of the type set passed.
     *
     * To prevent ambiguities, and unexpected function calls, this function
     * only participates in overload resolution when the type set is not the
     * exact same as the current type set.  Although it should not make a
     * difference in what the function does
     */
    template <typename TypeSetType,
              detail::EnableIfArentSame<TypeSet<Types...>, TypeSetType>*
                  = nullptr>
    TypeSet(TypeSetType&&) noexcept(sharp::AllOf_v<
            std::is_nothrow_move_constructible, std::tuple<Types...>>);

    /**
     * Move and copy assignment operators, one for the same types and one that
     * accepts a forwarding reference to any type, disabled here from overload
     * resolution to avoid confusion
     *
     * Three overloads for assignment operators are needed because the
     * forwarding reference assignment operator by itself would not be enough.
     * If I had just the forwarding reference assignment operator then the
     * rest of the assignment operators would be implicitly defined and that
     * would lead to surprising behavior
     */
    /**
     * The copy assignment operator copy assigns the instances from the other
     * type set into the current one.  Copying of elements is done in the
     * order that their types appeared in the type list that was used to
     * instantiate this class
     */
    TypeSet& operator=(const TypeSet&);

    /**
     * The move assignment operator move assigns the instances from the other
     * type set into the current one.  Moving of elements is done in the order
     * that their types appeared in the type list that was used to instantiate
     * this class
     */
    TypeSet& operator=(TypeSet&&) noexcept(sharp::AllOf_v<
            std::is_nothrow_move_assignable, std::tuple<Types...>>);

    /**
     * The forwarding reference assignment operator assigns elements from the
     * other type into the current one in the order that they appeared in the
     * type list that was used to instantiate the current class.
     *
     * To prevent ambiguities, this only participates in overload resolution
     * when the other type set is not the exact type as the current one.
     * Although it should not make a difference with respect to the behavior
     * of the class
     */
    template <typename TypeSetType,
              detail::EnableIfArentSame<TypeSet<Types...>, TypeSetType>*
                  = nullptr>
    TypeSet& operator=(TypeSetType&&) noexcept(sharp::AllOf_v<
            std::is_nothrow_move_assignable, std::tuple<Types...>>);

    /**
     * Destroys all the objects stored in the type set, this will destroy the
     * objects in the order that they were created.
     *
     * The order of creation corresponds to the order in which the elements
     * were given to the template parameters
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
     * MatchReference_t here because of CLANG BUG, I would use
     * decltype(auto) but it does not seem to work, try it goo.gl/kNmJIr
     *
     * Adding a noexcept specification to this function makes the current
     * version of clang that I am running quit with a segmentation fault,
     * sharp::AllOf_v<detail::IsNothrowConstructibleAllWays, tuple<Types...>>
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
     * Implementation constructor, this will be used by all the constructors
     * to provide default behavior
     */
    template <typename TypeSetType,
              detail::EnableIfAreTypeSets<TypeSet<Types...>,
                                          std::decay_t<TypeSetType>>* = nullptr>
    TypeSet(TypeSetType&&, sharp::implementation::tag_t) noexcept(
            sharp::AllOf_v<std::is_nothrow_move_constructible,
                           std::tuple<Types...>>);

    /**
     * Implementation assignment operator, this will be used by all the
     * assignment operators to provide default behavior
     */
    template <typename TypeSetType,
              detail::EnableIfAreTypeSets<TypeSet<Types...>,
                                          std::decay_t<TypeSetType>>* = nullptr>
    TypeSet& assign(TypeSetType&&) noexcept(sharp::AllOf_v<
            std::is_nothrow_move_assignable, std::tuple<Types...>>);

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
    static_assert(IsInstantiationOf_v<TransformedList, std::tuple>, "");
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

/**
 * @class NamedArgument
 *
 * A CRTP base class that exposes functions that can be used to make a type a
 * simple distinguishable alias for another type.  For example a function
 * accepts three possible parameters, of types Address, Port and Connection,
 * Address and Connection will be of type string whereas Port will be of type
 * integer, then a user can do the following
 *
 *      class Address : public sharp::NamedArgument<std::string> {
 *      public:
 *          Address(std::string input) :
 *              sharp::NamedArgument<std::string>{std::move(input)} {}
 *      };
 *
 *      class Port : public sharp::NamedArgument<int> {
 *      public:
 *          Port(int port = 80) : sharp::NamedArgument<int>{port} {}
 *      };
 *
 *      class Connection : public sharp::NamedArgument<std::string> {
 *      public:
 *          Connection(std::string input) :
 *              sharp::NamedArgument<std::string>{std::move(input)} {}
 *      };
 *
 * Since C++17 and beyond one can just do
 *
 *      class Address : public std::optional<std::string> {};
 *      class Port : public std::optional<int> {};
 *      class Connection : public std::optional<std::string> {};
 *
 * And then in the function that requires those arguments
 *
 *      template <typename... Args>
 *      void start_server(Args&&... args) {
 *          auto args = sharp::collect_args<Address, Port, Connection>(args...);
 *          cout << "Address : " << sharp::get<Address>(args).value() << endl;
 *          cout << "Port : " << sharp::get<Port>(args).value() << endl;
 *          cout << "Conn : " << sharp::get<Connection>(args).value() << endl;
 *
 *          // if using C++17 and beyond
 *          cout << "Port : " << sharp::get<Port>(args).value_or(80) << endl;
 *      }
 *
 */
template <typename Type>
class NamedArgument;

} // namespace sharp

#include <sharp/TypeSet/TypeSet.ipp>
