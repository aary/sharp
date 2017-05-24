/**
 * @file Range.hpp
 * @author Aaryaman Sagar
 *
 * A simple range module like the one in Python, it can be used with the range
 * based for loop syntax and has the same semantics as the one in Python.
 *
 * Just for fun ¯\_(ツ)_/¯
 */

#pragma once

#include <iterator>
#include <type_traits>

namespace sharp {

/**
 * The range function that should be embedded in a range based for loop like
 * so
 *
 *      for (auto ele : range(0, 10)) { ... }
 *
 * This will return a proxy that can support increments on its iterators till
 * the entire range has been covered, in the optimized builds, the extra
 * inefficiencies from creating objects and iterators will likely be optimized
 * away and the result will be suprisingly similar to a hand written for loop
 */
template <typename One, typename Two>
auto range(One begin, Two end);

/**
 * Privates!
 */
namespace detail {

    /**
     * Forward declaration for the class Range that is going to be used as the
     * actual range module, this should never explicitly be instantiated and
     * hence is in the detail namespace
     */
    template <typename One, typename Two>
    class Range {
    public:

        /**
         * Friend the factory function to create objects of this class
         */
        friend auto sharp::range<One, Two>(One, Two);

        template <typename IncrementableType>
        class Iterator {
        public:

            /**
             * Iterator categories, STL algorithms dont work on some
             * implementations without these
             */
            using difference_type = int;
            using value_type = IncrementableType;
            using pointer = std::add_pointer_t<IncrementableType>;
            using reference = std::add_lvalue_reference_t<IncrementableType>;
            using iterator_category = std::forward_iterator_tag;

            /**
             * The constructor for the iterator, adds the incrementable object
             * as a member variable
             */
            Iterator(IncrementableType);

            /**
             * Increment operator for the iterator
             */
            Iterator& operator++();

            /**
             * Not equals operator
             */
            template <typename Incrementable>
            bool operator!=(Iterator<Incrementable> other) const;

            /**
             * Return the thing that the iterator points to by value
             */
            decltype(auto) operator*() const;

        private:
            IncrementableType incrementable;
        };

        /**
         * Begin and end iterators for the range
         */
        Iterator<One> begin() const;
        Iterator<Two> end() const;

    private:
        /**
         * Private constructor for this class, can be constructed only with
         * the range() function defined below
         */
        explicit Range(One, Two);

        /**
         * The members to keep track of what the starting and ending points
         * were, these cannot be compiled away with constexpr template
         * varaibles because there can be times when the user wants dynamic
         * variables as starting and ending points
         */
        One first;
        Two last;
    };

} // namespace detail
} // namespace sharp

#include <sharp/Range/Range.ipp>
