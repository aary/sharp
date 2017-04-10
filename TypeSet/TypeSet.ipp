#pragma once

#include <tuple>
#include <utility>
#include <type_traits>

#include <sharp/Traits/Traits.hpp>
#include <sharp/TypeSet/TypeSet.hpp>

namespace sharp {

namespace detail {

    /**
     * Returns a reference to the instance of the right type as represented by
     * the context.  This reference is an lvalue reference, this function does
     * not forward the return type.  That should be done in the
     * execute_on_appropriate_tuple_element function
     *
     * If you strip out the template metaprogramming in this function.  All it
     * does is reinterpret_cast the aligned storage in the tuple to the type
     * passed in the context
     */
    template <typename Context, typename TupleType>
    sharp::MatchReference_t<TupleType&, typename Context::type>
    get_reference(Context, TupleType& tup) {

        // get the type that the current iteration is over, match constness
        using Type = typename Context::type;
        using TypeMatched = std::conditional_t<
            std::is_const<TupleType>::value, std::add_const_t<Type>, Type>;

        // get the type that the tuple would be of, this would not include
        // const qualifiers if the original type is not const so don't bother
        // with the type that has the const added to match the constness of
        // the reference to the tuple
        using TransformedType = typename AlignedStorageFor<Type>::type;

        // get the type of the tuple stored internally
        using Tuple = std::decay_t<TupleType>;

        // then get the index of that type in the current tuple
        constexpr auto index_type = sharp::FindIndex_v<TransformedType, Tuple>;
        static_assert(index_type < std::tuple_size<Tuple>::value,
            "index_type out of bounds");

        return *reinterpret_cast<TypeMatched*>(&std::get<index_type>(tup));
    }

    /**
     * Executes the passed in function on the appropriate element in the
     * tuple.  This is determined by the context passed to the function in
     * combination with the type of the tuple.
     *
     * The ForEach trait calls a function with a context variable as the
     * argument.  This context variable stores the type of the thing the
     * iteration is on currently as a static member typedef, so it can be
     * fetched with (typename decltype(context)::type).  This is done instead
     * of making the type a template parameter because this approach scales
     * without hassle to lambdas as well (which can only deduce type contexts)
     *
     * After that the location of this type is inspected in the tuple type and
     * then std::get<> is called with the appropriate index in the tuple and
     * that element is passed to the function passed
     *
     * This function matches the reference type of the tuple when passing the
     * argument to the functor passed.  For example if the tuple is an rvalue
     * (which would correspond tup being an rvalue reference, either to an
     * xvalue or to a prvalue) then the parameter if passed to func as if
     * moved by std::move().  Otherwise if tup is an lvalue referencem then
     * the object within tup is passed as an lvalue to the functor
     */
    template <typename Context, typename TupleType, typename Func>
    decltype(auto) execute_on_appropriate_tuple_element(Context context,
                                                        TupleType&& tup,
                                                        Func&& func) {
        // get the storage, this will be an lvalue reference
        auto& storage = get_reference(context, tup);

        // get the type that you need to static_cast the object to, this would
        // be either an rvalue reference type or an lvalue reference type,
        // following the template deduction rules, if a template parameter is
        // a forwarding reference, then its type will be & and if it is an
        // rvalue reference then its type will be &&, therefore TupleType can
        // either be qualified with a single & meaning that tup is an lvalue
        // reference or it can be qualified with nothing meaning that it is an
        // rvalue reference.  Therefore if TupleType is an lvalue reference
        // then cast the type to an lvalue reference
        using Type = std::decay_t<typename Context::type>;
        static_assert(std::is_same<Type, std::decay_t<decltype(storage)>>
                ::value, "Type mismatch in internal implementation");
        using TypeToCastTo = sharp::MatchReference_t<TupleType&&, Type>;

        // then conditionally cast the storage to the right type, and store
        // that in a forwarding reference, to forward in the next line
        auto&& storage_qualified = static_cast<TypeToCastTo>(storage);

        // call the functor after forwarding it to create an instance and then
        // pass the parameter after forwarding it into the function
        return std::forward<Func>(func)(
                std::forward<decltype(storage_qualified)>(storage_qualified));
    }

} // namespace detail

template <typename... Types>
TypeSet<Types...>::TypeSet() {

    sharp::ForEach<std::tuple<Types...>>{}([this](auto context) {
        // execute the function below on the right storage item, this storage
        // item will match the type context (which contains the type as a
        // typedef) passed
        detail::execute_on_appropriate_tuple_element(context,
                this->aligned_tuple, [context](auto&& storage) {
            static_assert(std::is_lvalue_reference<decltype(storage)>::value,
                "Wrong reference qualifiers were passed");
            static_assert(std::is_same<std::decay_t<decltype(storage)>,
                typename decltype(context)::type>::value, "Type mismatch");
            new (&storage) typename decltype(context)::type{};
        });
    });
}

template <typename... Types>
TypeSet<Types...>::TypeSet(sharp::empty::tag_t) {}

template <typename... Types>
TypeSet<Types...>::TypeSet(const TypeSet& other)
        : TypeSet{other, sharp::implementation::tag} {}

template <typename... Types>
TypeSet<Types...>::TypeSet(TypeSet&& other) noexcept(sharp::AllOf_v<
            std::is_nothrow_move_constructible, std::tuple<Types...>>)
        : TypeSet{std::move(other), sharp::implementation::tag} {}

template <typename... Types>
template <typename TypeSetType,
          detail::EnableIfArentSame<TypeSet<Types...>,
                                    TypeSetType>*>
TypeSet<Types...>::TypeSet(TypeSetType&& other) noexcept(sharp::AllOf_v<
            std::is_nothrow_move_constructible, std::tuple<Types...>>)
        : TypeSet{std::forward<TypeSetType>(other), sharp::implementation::tag}
{}

template <typename... Types>
template <typename TypeSetType,
          detail::EnableIfAreTypeSets<TypeSet<Types...>,
                                      std::decay_t<TypeSetType>>*>
TypeSet<Types...>::TypeSet(TypeSetType&& other, sharp::implementation::tag_t)
        noexcept(sharp::AllOf_v<std::is_nothrow_move_constructible,
                                std::tuple<Types...>>) {

    // move if this is true otherwise copy all the elements over to provide
    // strong exception guarantees
    constexpr auto should_move = sharp::AllOf_v<
        std::is_nothrow_move_constructible, std::tuple<Types...>>;

    sharp::ForEach<std::tuple<Types...>>{}([this, &other](auto context) {
        // execute a move operation on the other type if the type is move
        // constructible without throwing, otherwise copy the element
        using Type = typename decltype(context)::type;
        auto& other_element = sharp::get<Type>(other);
        auto& this_element = sharp::get<Type>(*this);
        if (should_move) {
            new (&this_element) Type{std::move(other_element)};
        } else {
            new (&this_element) Type{other_element};
        }
    });
}

template <typename... Types>
TypeSet<Types...>::~TypeSet() {

    sharp::ForEach<std::tuple<Types...>>{}([this](auto context) {
        // execute the function below on the right storage item, this storage
        // item will match the type context (which contains the type as a
        // typedef) passed
        detail::execute_on_appropriate_tuple_element(context,
                this->aligned_tuple, [context](auto& storage) {
            using Type = typename decltype(context)::type;
            static_assert(std::is_lvalue_reference<decltype(storage)>::value,
                "Wrong reference qualifiers were passed");
            static_assert(std::is_same<std::decay_t<decltype(storage)>,
                typename decltype(context)::type>::value, "Type mismatch");
            (&storage)->~Type();
        });
    });
}

template <typename Type, typename TypeSetType>
sharp::MatchReference_t<TypeSetType&&, Type> get(TypeSetType&& type_set) {

    // make the type context that is going to be used to query the internal
    // type set for the appropriate type and then the function passed to the
    // query function execute_on_appropriate_tuple_element is going to be
    // forwarded the internal data member of the tuple
    auto context = sharp::Identity<Type>{};
    return detail::execute_on_appropriate_tuple_element(context,
            std::forward<TypeSetType>(type_set).aligned_tuple,
            [context](auto&& storage) -> decltype(auto) {
        static_assert(std::is_same<std::decay_t<decltype(storage)>,
            typename decltype(context)::type>::value, "Type mismatch");
        return std::forward<decltype(storage)>(storage);
    });
}

template <typename... Types>
template <typename Type>
constexpr bool TypeSet<Types...>::exists() {
    return !std::is_same<sharp::Find<Type, std::tuple<Types...>>,
                         std::tuple<>>::value;
}

template <typename... Types, typename... Args>
TypeSet<Types...> collect_args(Args&&... args_list) /* noexcept( */
        // sharp::AllOf_v<detail::IsNothrowConstructibleAllWays,
                       /* std::tuple<Types...>>) */ {

    // assert the type list invariants
    static_assert(detail::check_type_list_invariants<Types...>,
            "Type list malformed");

    // create the empty type instance and also store the function arguments
    // in a tuple ready to be forwarded
    //
    // until mandatory copy elision is a thing, a library should not rely on
    // that for correct behavior, therefore none of the following
    //
    //  auto instance = TypeSet<Types...>{sharp::empty::tag};
    //
    TypeSet<Types...> instance{sharp::empty::tag};
    auto args = std::forward_as_tuple(std::forward<Args>(args_list)...);
    using ArgumentTypes = sharp::Transform_t<std::decay, std::tuple<Args...>>;

    // then iterate through the types
    sharp::ForEach<std::tuple<Types...>>{}([&args, &instance](auto context) {

        // if the type is in the argument list of arguments then get the
        // argument out of the arguments and use that to construct the element
        // in the typeset
        using CurrentType = typename decltype(context)::type;

        // get the instance that needs to be constructed
        auto& storage = sharp::get<CurrentType>(instance);

        // would use constexpr if but can't, anyway most compilers should
        // optimize this away since the thing in the if is evaluated at
        // compile time
        constexpr auto index_in_type_set = sharp::FindIndex_v<CurrentType,
                                                              ArgumentTypes>;
        if (index_in_type_set < std::tuple_size<ArgumentTypes>::value) {

            // get the reference qualified version of the tuple element at
            // which the current type resides
            using TupleElement = std::tuple_element_t<
                index_in_type_set,
                std::decay_t<decltype(args)>>;
            new (&storage) CurrentType{std::forward<TupleElement>(
                std::get<index_in_type_set>(args))};
        } else {
            new (&storage) CurrentType{};
        }
    });

    return instance;
}

template <typename... Types>
TypeSet<Types...>& TypeSet<Types...>::operator=(const TypeSet& other) {
    return this->assign(other);
}

template <typename... Types>
TypeSet<Types...>& TypeSet<Types...>::operator=(TypeSet&& other)
        noexcept(sharp::AllOf_v<std::is_nothrow_move_assignable,
                                std::tuple<Types...>>) {
    return this->assign(std::move(other));
}

template <typename... Types>
template <typename TypeSetType,
          detail::EnableIfArentSame<TypeSet<Types...>,
                                    TypeSetType>*>
TypeSet<Types...>& TypeSet<Types...>::operator=(TypeSetType&& other)
        noexcept(sharp::AllOf_v<std::is_nothrow_move_assignable,
                                std::tuple<Types...>>) {
    return this->assign(std::forward<TypeSetType>(other));
}

template <typename... Types>
template <typename TypeSetType,
          detail::EnableIfAreTypeSets<TypeSet<Types...>,
                                      std::decay_t<TypeSetType>>*>
TypeSet<Types...>& TypeSet<Types...>::assign(TypeSetType&& other) noexcept(
        sharp::AllOf_v<std::is_nothrow_move_assignable, std::tuple<Types...>>) {

    // move if this is true otherwise copy all the elements over to provide
    // strong exception guarantees
    constexpr auto should_move = sharp::AllOf_v<
        std::is_nothrow_move_assignable, std::tuple<Types...>>;

    sharp::ForEach<std::tuple<Types...>>{}([&other, this](auto context) {
        // assert that calling get<> on an rvalue type returns an rvalue
        // reference
        using Type = typename decltype(context)::type;
        static_assert(std::is_same<
            decltype(sharp::get<Type>(std::move(other))), Type&&>::value,
            "sharp::get<> evaluated to the wrong value category");

        if (should_move) {
            sharp::get<Type>(*this) = sharp::get<Type>(std::move(other));
        } else {
            sharp::get<Type>(*this) = sharp::get<Type>(other);
        }
    });

    return *this;
}

} // namespace sharp
