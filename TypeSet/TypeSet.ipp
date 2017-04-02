#pragma once

#include <tuple>
#include <type_traits>

#include <sharp/Traits/Traits.hpp>
#include <sharp/TypeSet/TypeSet.hpp>

namespace sharp {

template <typename... Types>
TypeSet<Types...>::TypeSet() {
    sharp::ForEach<std::tuple<Types...>>{}([&](auto context) mutable {

        // get the type that the current iteration is over
        using Type = typename decltype(context)::type;
        using TransformedType = typename detail::AlignedStorageFor<Type>::type;

        // get the type of the tuple stored internally
        using TupleType = std::decay_t<decltype(this->aligned_tuple)>;

        // then get the index of that type in the current tuple
        constexpr auto index_type =
            std::tuple_size<TupleType>::value
            - std::tuple_size<sharp::Find_t<TransformedType, TupleType>>::value;
        static_assert(index_type < std::tuple_size<TupleType>::value,
            "index_type out of bounds");

        auto* storage = &(std::get<index_type>(this->aligned_tuple)).storage;
        new (storage) Type{};
    });
}

template <typename... Types>
TypeSet<Types...>::~TypeSet() {
    sharp::ForEach<std::tuple<Types...>>{}([&](auto context) mutable {

        // get the type that the current iteration is over
        using Type = typename decltype(context)::type;
        using TransformedType = typename detail::AlignedStorageFor<Type>::type;

        // get the type of the tuple stored internally
        using TupleType = std::decay_t<decltype(this->aligned_tuple)>;

        // then get the index of that type in the current tuple
        constexpr auto index_type =
            std::tuple_size<TupleType>::value
            - std::tuple_size<sharp::Find_t<TransformedType, TupleType>>::value;
        static_assert(index_type < std::tuple_size<TupleType>::value,
            "index_type out of bounds");

        auto* storage = &(std::get<index_type>(this->aligned_tuple)).storage;
        reinterpret_cast<Type*>(storage)->~Type();
    });
}

} // namespace sharp
