#include <sharp/Range/Range.hpp>

namespace sharp {

template <typename One, typename Two>
auto range(One begin, Two end) {
    return detail::Range<One, Two>{begin, end};
}

namespace detail {

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

    template <typename One, typename Two>
    template <typename IncrementableType>
    Range<One, Two>::Iterator<IncrementableType>::Iterator(
            IncrementableType incrementable_in)
        : incrementable{incrementable_in} {}

    template <typename One, typename Two>
    template <typename IncrementableType>
    IncrementableType Range<One, Two>:: Iterator<IncrementableType>::operator*()
            const {
        return this->incrementable;
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
