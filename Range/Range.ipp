#include <sharp/Range/Range.hpp>

namespace sharp {

template <typename One, typename Two>
auto range(One begin, Two end) {
    return detail::Range<One, Two>{begin, end};
}

/**
 * All the major implementation goes in the detail namespace
 */
namespace detail {

    /**
     * Concepts
     */
    template <typename Type>
    using EnableIfIsIterator = std::enable_if_t<std::is_same<
        typename std::decay_t<Type>::iterator_category,
        typename std::decay_t<Type>::iterator_category>::value>;

    /**
     * Compile time switch that returns the dereferneced version of the range
     * if the type is an iterator, whether or not the element is an iterator
     * is tested via the presence of the iterator_category trait
     */
    template <typename IncrementableType, typename = std::enable_if_t<true>>
    struct Dereference {
        template <typename Incrementable>
        static auto dereference(Incrementable&& i) {
            return i;
        }
    };
    template <typename IncrementableType>
    struct Dereference<IncrementableType,
                       EnableIfIsIterator<IncrementableType>> {
        template <typename Incrementable>
        static decltype(auto) dereference(Incrementable&& i) {
            return *std::forward<Incrementable>(i);
        }
    };

    /**
     * Member functions of the main proxy Range
     */
    template <typename One, typename Two>
    Range<One, Two>::Range(One begin_in, Two end_in)
        : first{begin_in}, last{end_in} {}

    template <typename One, typename Two>
    Range<One, Two>::Iterator<One> Range<One, Two>:: begin() const {
        return Iterator<One>{this->first};
    }

    template <typename One, typename Two>
    Range<One, Two>::Iterator<Two> Range<One, Two>::end() const {
        return Iterator<Two>{this->last};
    }


    /**
     * Member functions for the iterator type
     */
    template <typename One, typename Two>
    template <typename IncrementableType>
    Range<One, Two>::Iterator<IncrementableType>::Iterator(
            IncrementableType incrementable_in)
        : incrementable{incrementable_in} {}

    template <typename One, typename Two>
    template <typename IncrementableType>
    decltype(auto) Range<One, Two>:: Iterator<IncrementableType>::operator*()
            const {
        return Dereference<IncrementableType>::dereference(this->incrementable);
    }

    template <typename One, typename Two>
    template <typename IncrementableType>
    Range<One, Two>::Iterator<IncrementableType>&
    Range<One, Two>::Iterator<IncrementableType>::operator++() {
        ++this->incrementable;
        return *this;
    }

    template <typename One, typename Two>
    template <typename IncrementableType>
    template <typename Incrementable>
    bool Range<One, Two>::Iterator<IncrementableType>::operator!=(
            Iterator<Incrementable> other) const {
        return this->incrementable != other.incrementable;
    }

} // namespace detail

} // namespace sharp
