#include <sharp/TransparentList/TransparentList.hpp>
#include <sharp/Defer/Defer.hpp>

#include <cassert>
#include <iterator>
#include <cstdint>
#include <iostream>
#include <type_traits>

namespace sharp {

template <typename Type>
static bool is_satisfying_alignment_invariants(Type* pointer) {
    // assert that the pointer is aligned on the right boundary with respect
    // to the largest alignment on the system just to be safe
    return !(reinterpret_cast<std::uintptr_t>(pointer)
                % alignof(std::max_align_t));
}

/**
 * An iterator class for generality and convenience
 */
template <typename Type>
class TransparentList<Type>::NodeIterator {
public:

    /**
     * These are required to maintain compatibility with STL algorithms
     *
     * difference_type denotes a type that can be used to denote the
     * distance between 2 iterators, see std::distance
     *
     * value_type is equivalent to
     *  std::decay_t<*std::declval<NodeIterator<Type>>()>
     *
     * pointer is equivalent to std::add_pointer_t<value_type>
     *
     * reference equivalent to std::add_reference_t<value_type>
     *
     * iterator_category represents the type of iterator, it can be either
     * a random access iterator, a forward iterator, a bidirectional
     * iterator, a reverse iterator, etc
     */
    using difference_type = int;
    using value_type = TransparentNode<Type>*;
    using pointer = TransparentNode<Type>**;
    using reference = std::add_lvalue_reference_t<TransparentNode<Type>*>;
    using iterator_category = std::bidirectional_iterator_tag;

    /**
     * Dereferenece operator
     */
    TransparentNode<Type>* operator*() noexcept {
        assert(is_satisfying_alignment_invariants(this->node_ptr));
        assert(this->node_ptr);
        return this->node_ptr;
    }

    /**
     * Preincrement operator to advance the node pointer to the next spot
     */
    NodeIterator& operator++() noexcept {
        assert(is_satisfying_alignment_invariants(this->node_ptr));
        assert(this->node_ptr);
        this->node_ptr = this->node_ptr->next;
        return *this;
    }

    /**
     * Predecrement operator to move the node pointer to the previous spot
     */
    NodeIterator& operator--() noexcept {
        assert(is_satisfying_alignment_invariants(this->node_ptr));
        assert(this->node_ptr);
        this->node_ptr = this->node_ptr->prev;
        return *this;
    }

    /**
     * Assignment operator to assign to and from an iterator
     */
    NodeIterator& operator=(const NodeIterator& other) noexcept {
        assert(is_satisfying_alignment_invariants(other.node_ptr));
        this->node_ptr = other.node_ptr;
        return *this;
    }

    /**
     * Equality operators to determine whether two iterators are pointing to
     * the same node
     */
    bool operator==(const NodeIterator& other) const noexcept {
        assert(is_satisfying_alignment_invariants(this->node_ptr));
        assert(is_satisfying_alignment_invariants(other.node_ptr));
        return this->node_ptr == other.node_ptr;
    }
    bool operator!=(const NodeIterator& other) const noexcept {
        assert(is_satisfying_alignment_invariants(this->node_ptr));
        assert(is_satisfying_alignment_invariants(other.node_ptr));
        return this->node_ptr != other.node_ptr;
    }

    /**
     * Become friends with the list class that this iterator is a part of
     * so that the list class can access the constructor for this class
     */
    friend class TransparentList<Type>;

private:

    /**
     * The only way to construct an iterator that points to a Node is by
     * passing a valid node pointer, an assertion fails if the node
     * pointer is null
     */
    NodeIterator(TransparentNode<Type>* node_in) noexcept : node_ptr{node_in} {
        assert(is_satisfying_alignment_invariants(this->node_ptr));
    }

    /**
     * A pointer to the node that the iterator refers to
     */
    TransparentNode<Type>* node_ptr;
};

/**
 * Implementations for the linked list class methods
 */
template <typename Type>
auto TransparentList<Type>::begin() noexcept {
    return NodeIterator{this->head};
}

template <typename Type>
auto TransparentList<Type>::end() noexcept {
    return NodeIterator{nullptr};
}

template <typename Type>
TransparentList<Type>::TransparentList() noexcept = default;

template <typename Type>
void TransparentList<Type>::insert_after(
        TransparentNode<Type>* to_insert_after,
        TransparentNode<Type>* to_insert) noexcept {
    // insert right after the node
    to_insert->prev = to_insert_after;
    to_insert->next = to_insert_after->next;

    // change the next node's previous pointer if the next pointer is valid
    if (to_insert_after->next) {
        to_insert_after->next->prev = to_insert;
    }

    // then actually change the next pointer of the previous node to point to
    // the current node
    to_insert_after->next = to_insert;
}

template <typename Type>
void TransparentList<Type>::insert_before(
        TransparentNode<Type>* to_insert_before,
        TransparentNode<Type>* to_insert) noexcept {
    // insert right before the node
    to_insert->prev = to_insert_before->prev;
    to_insert->next = to_insert_before;

    // change the previous node's next pointer if the previous pointer is valid
    if (to_insert_before->prev) {
        to_insert_before->prev->next = to_insert;
    }

    // then change the actual previous pointer to point to this node
    to_insert_before->prev = to_insert;
}

template <typename Type>
void TransparentList<Type>::push_back(TransparentNode<Type>* node_to_insert)
        noexcept {
    assert(is_satisfying_alignment_invariants(node_to_insert));
    assert(node_to_insert);

    // handle base case when the tail pointer is pointing to nothing
    if (!this->tail) {
        // if the tail pointer is pointing to nothing then the head should
        // also be pointing to null
        assert(!this->head);
        node_to_insert->prev = nullptr;
        node_to_insert->next = nullptr;
        this->tail = node_to_insert;
        this->head = node_to_insert;
        return;
    }

    // otherwise just insert into the linked list
    assert(!this->tail->next);
    this->insert_after(this->tail, node_to_insert);
    this->tail = node_to_insert;
}

template <typename Type>
void TransparentList<Type>::push_front(TransparentNode<Type>* node_to_insert)
        noexcept {
    assert(is_satisfying_alignment_invariants(node_to_insert));
    assert(node_to_insert);

    // if the head pointer it pointing to null then just insert in the
    // beginning
    if (!this->head) {
        // if the tail pointer is pointing to nothing then the head should
        // also be pointing to null
        assert(!this->tail);
        this->push_back(node_to_insert);
        assert(this->tail == this->head);
        return;
    }

    // otherwise just insert into the beginning
    assert(!this->head->prev);
    this->insert_before(this->head, node_to_insert);
    this->head = node_to_insert;
}

template <typename Type>
typename TransparentList<Type>::NodeIterator
TransparentList<Type>::insert(TransparentList<Type>::NodeIterator iterator,
                              TransparentNode<Type>* node_to_insert) noexcept {
    assert(is_satisfying_alignment_invariants(node_to_insert));
    assert(node_to_insert);

    // if the iterator was pointing past the end of the linked list then
    // insert should insert past the last element, i.e. a push_back()
    if (iterator == this->end()) {
        this->push_back(node_to_insert);
        return NodeIterator{node_to_insert};
    }

    // if not then the iterator was pointing to a valid element, insert right
    // before it, at this stage there was only one node so insert right before
    // it.  This can change the head pointer since it is a "before this"
    // insertion so update the head also
    assert(iterator.node_ptr);
    this->insert_before(iterator.node_ptr, node_to_insert);
    if (iterator == this->begin()) {
        this->head = this->head->prev;
    }
    return NodeIterator{node_to_insert};
}

template <typename Type>
typename TransparentList<Type>::NodeIterator
TransparentList<Type>::erase(TransparentList<Type>::NodeIterator iterator)
        noexcept {
    assert(is_satisfying_alignment_invariants(iterator.node_ptr));
    assert(iterator.node_ptr);
    auto iterator_to_return = NodeIterator{iterator.node_ptr->next};

    // Make the next and previous pointers off the current one point over the
    // removed node
    if (iterator.node_ptr->next) {
        iterator.node_ptr->next->prev = iterator.node_ptr->prev;
    }
    if (iterator.node_ptr->prev) {
        iterator.node_ptr->prev->next = iterator.node_ptr->next;
    }

    // if this was the only node in the list then erase it and set the head
    // and tail pointers to null
    if (this->head == iterator.node_ptr) {
        this->head = this->head->next;

        // if the tail was equal to the head then there was only one element
        if (this->tail == iterator.node_ptr) {
            assert(!this->head);
            this->tail = nullptr;
        }
        return iterator_to_return;
    }
    assert(this->tail != this->head);
    return iterator_to_return;
}

template <typename Type>
template <typename OtherList>
void TransparentList<Type>::splice(TransparentList<Type>::NodeIterator iterator,
                                   OtherList&& other) {
    // if other is empty then just return
    if (!other.head) {
        return;
    }

    // after this other should be empty when returning
    auto deferred = sharp::defer([&] {
        other.head = nullptr;
        other.tail = nullptr;
    });

    // there are two cases to consider, the first is when inserting the node
    // at the end, and the other is when inserting the node anywhere else in
    // the list
    if (iterator == this->end()) {
        auto previous_tail = this->tail;
        this->tail = other.tail;
        if (previous_tail) {
            assert(!previous_tail->next);
            previous_tail->next = other.head;
            assert(!other.head->prev);
            other.head->prev = previous_tail;
        } else {
            assert(!this->head);
            this->head = other.head;
            this->tail = other.tail;
        }
    }

    // if inserting anywhere else only the head pointer may be changed
    else {
        auto previous_current = *iterator;
        if (previous_current->prev) {
            assert(previous_current->prev->next == previous_current);
            previous_current->prev->next = other.head;
            assert(!other.head);
            other.head->prev = previous_current->prev;
        } else {
            this->head = other.head;
        }

        assert(!other.tail->next);
        other.tail->next = previous_current;
        previous_current->prev = other.tail;
    }
}

} // namespace sharp
