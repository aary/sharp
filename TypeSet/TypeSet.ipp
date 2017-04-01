#pragma once

#include <tuple>
#include <type_traits>

#include <sharp/Traits/Traits.hpp>
#include <sharp/TypeSet/TypeSet.hpp>

namespace sharp {

template <typename... Types>
TypeSet<Types...>::TypeSet() {
    sharp::for_each_tuple(this->aligned_tuple, [](auto& storage) {
        // get the type that the storage item is representing
        using type = typename std::decay_t<decltype(storage)>::type;

        // find the index of the type that the storage item is representing in
        // the template pack
        // constexpr auto index_in_type_set =
            // std::tuple_size<std::tuple<Types...>>::value
            // - std::tuple_size<sharp::Find_t<type, Types...>>::value;

        // then construct the item with the appropriate type
        new (&storage.storage) type{};
    });
}

template <typename... Types>
TypeSet<Types...>::~TypeSet() {
    sharp::for_each_tuple(this->aligned_tuple, [](auto& storage) {
        // get the type that the storage item is representing
        using type = typename std::decay_t<decltype(storage)>::type;

        // find the index of the type that the storage item is representing in
        // the template pack
        // constexpr auto index_in_type_set =
            // std::tuple_size<std::tuple<Types...>>::value
            // - std::tuple_size<sharp::Find_t<type, Types...>>::value;

        // then construct the item with the appropriate type
        reinterpret_cast<type*>(&storage.storage)->~type();
    });
}

} // namespace sharp
