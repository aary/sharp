#pragma once

#include <sharp/ForEach/ForEach.hpp>
#include <sharp/Traits/Traits.hpp>

#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>
#include <iterator>
#include <array>

// assert that both sharp::void_t and std::enable_if_t result in the same type
// becasue this library depends on that being true, concepts are described in
// terms of both and the call site sometimes requires either to be compatibe
// with the other
static_assert(std::is_same<sharp::void_t<>, std::enable_if_t<true>>::value, "");

namespace sharp {

namespace for_each_detail {

/**
 * An internal namespace that contains functions that do ADL lookups, for this
 * reason those functions that are looked up via ADL must be imported in the
 * namespace to provide backup in case the passed type does not contain
 * ADL defined functions
 */
namespace adl {

    /**
     * Import the 3 things that can be ADL defined for this library
     */
    using std::get;
    using std::begin;
    using std::end;

    /**
     * Function to simulate adl lookup on the begin, end and get<> functions
     */
    template <typename Range>
    auto adl_begin(Range&& range) -> decltype(begin(std::declval<Range>())) {
        return begin(std::forward<Range>(range));
    }
    template <typename Range>
    auto adl_end(Range&& range) -> decltype(end(std::declval<Range>())) {
        return end(range);
    }
    template <std::size_t Index, typename Range>
    auto adl_get(Range&& range) -> decltype(get<Index>(std::declval<Range>())) {
        return get<Index>(std::forward<Range>(range));
    }

} // namespace adl

    /**
     * Concepts to check if the range has a member get<> or if it has a free
     * function get
     */
    template <typename Range>
    using EnableIfNonMemberGetWorks = sharp::void_t<
        decltype(adl::adl_get<0>(std::declval<Range>()))>;
    template <typename Range>
    using EnableIfMemberGetWorks = sharp::void_t<
        decltype(std::declval<Range>().template get<0>())>;

    /**
     * Helper to either retrieve the value through a member get method or a
     * free get function
     */
    template <std::size_t Index, typename Range, typename = sharp::void_t<>>
    class Get {
    public:
        template <typename R>
        static auto impl(R&& range)
                -> decltype(adl::adl_get<Index>(std::declval<R>())) {
            return adl::adl_get<Index>(std::forward<R>(range));
        }
    };
    template <std::size_t Index, typename Range>
    class Get<Index, Range, EnableIfMemberGetWorks<Range>> {
    public:
        template <typename R>
        static auto impl(R&& range)
                -> decltype(adl::adl_get<Index>(std::declval<R>())) {
            return std::forward<R>(range).template get<Index>();
        }
    };

    /**
     * Concepts to check if the range is a compile time range or a runtime
     * range
     */
    template <typename Range>
    using EnableIfCompileTimeRange = sharp::void_t<
        decltype(Get<0, Range>::impl(std::declval<Range>())),
        decltype(std::tuple_size<std::decay_t<Range>>::value)>;
    template <typename Range>
    using EnableIfRuntimeRange = sharp::void_t<
        decltype(adl::adl_begin(std::declval<Range>())),
        decltype(adl::adl_end(std::declval<Range>()))>;

    /**
     * A simple overloaded trait to simulate the result of declval on the
     * range, this works both for compile time ranges (tuple like) and on
     * runtime ranges
     */
    template <typename Range, typename = sharp::void_t<>>
    class DeclvalSequence {
    public:
        static auto impl()
            -> decltype(*(adl::adl_begin(std::declval<Range>())));
    };
    template <typename Range>
    class DeclvalSequence<Range, EnableIfCompileTimeRange<Range>> {
    public:
        static auto impl()
            -> decltype(Get<0, Range>::impl(std::declval<Range>()));
    };

    /**
     * Enables if the function accepts the number of args required
     */
    template <typename Range, typename Func>
    using EnableIfAcceptsOneArg = sharp::void_t<
        decltype(std::declval<Func>()(
                    DeclvalSequence<Range>::impl()))>;
    template <typename Range, typename Func>
    using EnableIfAcceptsTwoArgs = sharp::void_t<
        decltype(std::declval<Func>()(
                    DeclvalSequence<Range>::impl(),
                    std::integral_constant<std::size_t, 0>{}))>;
    template <typename Range, typename Func>
    using EnableIfAcceptsThreeArgs = sharp::void_t<
        decltype(std::declval<Func>()(
                    DeclvalSequence<Range>::impl(),
                    std::integral_constant<std::size_t, 0>{},
                    adl::adl_begin(std::declval<Range>())))>;
    /**
     * Enables if the function returns a break condition
     */
    template <typename Range, typename Func>
    using EnableIfBreaksCompileTime = std::enable_if_t<std::is_same<
        std::decay_t<decltype(std::declval<Func>()(
                    DeclvalSequence<Range>::impl(),
                    std::integral_constant<std::size_t, 0>{}))>,
        std::decay_t<decltype(sharp::loop_break)>>::value>;
    template <typename Range, typename Func>
    using EnableIfBreaksRuntime = std::enable_if_t<std::is_same<
        std::decay_t<decltype(std::declval<Func>()(
                    DeclvalSequence<Range>::impl(),
                    std::integral_constant<std::size_t, 0>{},
                    adl::adl_begin(std::declval<Range>())))>,
        std::decay_t<decltype(sharp::loop_break)>>::value>;

    template <typename Range, typename Func, typename = sharp::void_t<>>
    class ForEachRuntimeImpl {
    public:
        template <typename R, typename F>
        static void impl(R&& range, F& func) {

            // iterate through the range, not using a range based for loop
            // because in the future this algorithm should pass iterators to
            // the functor as well
            //
            // this has a slight difference with the semantics of a loop with
            // respect to the range based for loop.  The range based for loop
            // does things slightly differently.  The range is not forwarded
            // to the begin and end methods, it is just passed via a bound
            // lvalue refernece, which means that if a class has decided to
            // overload the begin and end methods/functions to return move
            // iterators for rvalues range based for loops will not work, this
            // loop however will work and will move things in that case
            auto first = adl::adl_begin(std::forward<Range>(range));
            auto last = adl::adl_end(std::forward<Range>(range));
            for (auto index = std::size_t{0}; first != last; ++first, ++index) {
                func(*first, index, first);
            }
        }
    };
    template <typename Range, typename Func>
    class ForEachRuntimeImpl<Range, Func, EnableIfBreaksRuntime<Range, Func>> {
    public:
        template <typename R, typename F>
        static void impl(R&& range, F& func) {

            // iterate through the range, not using a range based for loop
            // because in the future this algorithm should pass iterators to
            // the functor as well
            //
            // this has a slight difference with the semantics of a loop with
            // respect to the range based for loop.  The range based for loop
            // does things slightly differently.  The range is not forwarded
            // to the begin and end methods, it is just passed via a bound
            // lvalue refernece, which means that if a class has decided to
            // overload the begin and end methods/functions to return move
            // iterators for rvalues range based for loops will not work, this
            // loop however will work and will move things in that case
            auto first = adl::adl_begin(std::forward<Range>(range));
            auto last = adl::adl_end(std::forward<Range>(range));
            auto has_broken = sharp::loop_continue;
            for (auto index = std::size_t{0}; first != last; ++first, ++index) {
                has_broken = func(*first, index, first);
                if (has_broken == sharp::loop_break) {
                    break;
                }
            }
        }
    };

    /**
     * Implementation of the for_each runtime algorithm, this has three cases,
     * one for the case where the function accepts one argument, another
     * for the case when the function accepts two arguments and the last for
     * when the function accepts an iterator as well
     */
    template <typename Range, typename Func,
              EnableIfAcceptsThreeArgs<Range, Func>* = nullptr>
    void for_each_runtime_impl(Range&& range, Func& func) {
        // forward implementation to the specialization that does different
        // things based on whether the function returns a break or not
        ForEachRuntimeImpl<Range, Func>::impl(std::forward<Range>(range), func);
    }
    template <typename Range, typename Func,
              EnableIfAcceptsTwoArgs<Range, Func>* = nullptr>
    void for_each_runtime_impl(Range&& range, Func& func) {

        // construct an adaptor that makes the function accept three arguments
        // instead of two and then pass that to the implementation function to
        // reduce code duplication
        auto three_arg_adaptor = [&func](auto&& ele, auto index, auto)
                -> decltype(auto) {
            return func(std::forward<decltype(ele)>(ele), index);
        };
        for_each_runtime_impl(std::forward<Range>(range), three_arg_adaptor);
    }
    template <typename Range, typename Func,
              EnableIfAcceptsOneArg<Range, Func>* = nullptr>
    void for_each_runtime_impl(Range&& range, Func& func) {

        // construct an adaptor that makes the function passed in a two
        // argument function and then pass that to the implementation function
        auto two_arg_adaptor = [&func](auto&& ele, auto) -> decltype(auto) {
            return func(std::forward<decltype(ele)>(ele));
        };
        for_each_runtime_impl(std::forward<Range>(range), two_arg_adaptor);
    }

    template <typename Range, typename Func, typename = sharp::void_t<>>
    class ForEachCompileTimeImpl {
    public:
        template <typename R, typename F, std::size_t... Indices>
        static void impl(R&& range, F& func,
                         std::integer_sequence<std::size_t, Indices...>) {

            // construct an initializer list and use its arguments to loop over
            // the function,
            static_cast<void>(std::initializer_list<int>{
                 (func(Get<Indices, Range>::impl(std::forward<Range>(range)),
                     std::integral_constant<std::size_t, Indices>{}), 0)...
            });
        }
    };
    template <typename Range, typename Func>
    class ForEachCompileTimeImpl<Range, Func,
                                 EnableIfBreaksCompileTime<Range, Func>> {
    public:
        template <typename R, typename F, std::size_t... Indices>
        static void impl(R&& range, F& func,
                         std::integer_sequence<std::size_t, Indices...>) {

            // the variable that is used as the break condition, ideally the
            // compiler should be able to optimize this and treat is as a
            // regular unrolled loop
            auto has_broken = sharp::loop_continue;

            // construct an initializer list and use its arguments to loop over
            // the function, use a ternary conditional to determine whether
            // the user has broken or not
            static_cast<void>(std::initializer_list<int>{
                (((has_broken == sharp::loop_continue) ?
                 (has_broken =
                  func(
                      Get<Indices, Range>::impl(std::forward<Range>(range)),
                      std::integral_constant<std::size_t, Indices>{})) :
                 (sharp::loop_continue)),
                0)...
            });
        }
    };

    template <typename Range, typename Func,
              EnableIfAcceptsTwoArgs<Range, Func>* = nullptr>
    void for_each_compile_time_impl(Range&& range, Func& func) {
        // compute the length and then pass that to another function which
        // will use the length and the integral_sequence associated with
        // that to loop through a range via pack expansion
        constexpr auto length = std::tuple_size<std::decay_t<Range>>::value;
        ForEachCompileTimeImpl<Range, Func>::impl(std::forward<Range>(range),
                func, std::make_index_sequence<length>{});
    }
    template <typename Range, typename Func,
              EnableIfAcceptsOneArg<Range, Func>* = nullptr>
    void for_each_compile_time_impl(Range&& range, Func& func) {

        // construct an adaptor that makes the function accept two arguments,
        // and then pass that to the other implementation function that deals
        // with functions accepting two arguments
        auto two_arg_adaptor = [&func](auto&& ele, auto) -> decltype(auto) {
            return func(std::forward<decltype(ele)>(ele));
        };
        for_each_compile_time_impl(std::forward<Range>(range), two_arg_adaptor);
    }

    /**
     * The internal class switches based on whether the passed in range is a
     * runtime range or a compile time range
     */
    template <typename Range, typename = sharp::void_t<>>
    class ForEachImpl {
    public:
        template <typename R, typename Func>
        static void impl(R&& range, Func& func) {
            for_each_compile_time_impl(std::forward<R>(range), func);
        }
    };
    template <typename Range>
    class ForEachImpl<Range, EnableIfRuntimeRange<Range>> {
    public:
        template <typename R, typename Func>
        static void impl(R&& range, Func& func) {
            for_each_runtime_impl(std::forward<R>(range), func);
        }
    };

    template <typename Range, typename Index, typename = sharp::void_t<>>
    class FetchImpl {
    public:
        template <typename R, typename I>
        static decltype(auto) fetch(R&& range, I index) {
            return Get<static_cast<std::size_t>(index), R>::impl(
                    std::forward<R>(range));
        }
    };
    template <typename Range, typename Index>
    class FetchImpl<Range, Index, EnableIfRuntimeRange<Range>> {
        template <typename R, typename I>
        static decltype(auto) fetch(R&& range, I index) {

            // assert that the range has random access iterators
            static_assert(has_random_access_iterators<Range>, "");
        }
    };

} // namespace detail

template <typename Range, typename Func>
constexpr Func for_each(Range&& tup, Func func) {
    for_each_detail::ForEachImpl<Range>::impl(std::forward<Range>(tup), func);
    return func;
}

template <typename Range, typename Index>
decltype(auto) fetch(Range&& range, Index index) {
    // dispatch to the implementation function that does different things
    // based on whether the range is a runtime range or a compile time range
    return for_each_detail::FetchImpl<Range>::impl(std::forward<Range>(range),
            index);
}

} // namespace sharp

