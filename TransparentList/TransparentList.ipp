#include <cassert>
#include <iterator>
#include <cstdint>
#include <iostream>

#include <sharp/TransparentList/TransparentList.hpp>

namespace sharp {

template <typename Type>
static void assert_pointer_invariants(Type* pointer) {
    // assert that the pointer is valid and not null
    assert(pointer);

    // assert that the pointer is aligned on the right boundary with respect
    // to the largest alignment on the system just to be safe
    assert(!(reinterpret_cast<uintptr_t>(pointer) % alignof(std::max_align_t)));
}

/**
 * Implementations of the iterator class methods
 */
/**
 * The dereference operator for the node iterator class
 */
template <typename Type>
auto& TransparentList<Type>::NodeIterator::operator*() noexcept {
    return *this->node_ptr;
}

template <typename Type>
Node<Type>* TransparentList<Type>::NodeIterator::operator->() noexcept {
    return this->node_ptr;
}

/**
 * Preincrement operator for the iterator to elements of the doubly linked list
 */
template <typename Type>
typename TransparentList<Type>::NodeIterator&
TransparentList<Type>::NodeIterator::operator++() noexcept {
    this->node_ptr = this->node_ptr->next;
    return *this;
}

/**
 * Comparison operators
 */
template <typename Type>
bool TransparentList<Type>::NodeIterator::operator==(
        const NodeIterator& other) const noexcept {
    return this->node_ptr == other.node_ptr;
}
template <typename Type>
bool TransparentList<Type>::NodeIterator::operator!=(
        const NodeIterator& other) const noexcept {
    return this->node_ptr != other.node_ptr;
}

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
TransparentList<Type>::TransparentList() noexcept
        : head{nullptr}, tail{nullptr} {}

template <typename Type>
void TransparentList<Type>::insert_after(Node<Type>* to_insert_after,
                                         Node<Type>* to_insert) noexcept {
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
void TransparentList<Type>::insert_before(Node<Type>* to_insert_before,
                                          Node<Type>* to_insert) noexcept {
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
void TransparentList<Type>::push_back(Node<Type>* node_to_insert) noexcept {
    assert_pointer_invariants(node_to_insert);

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
void TransparentList<Type>::push_front(Node<Type>* node_to_insert) noexcept {
    assert_pointer_invariants(node_to_insert);

    // if the head pointer it pointing to null then just insert in the
    // beginning
    if (!this->head) {
        // if the tail pointer is pointing to nothing then the head should
        // also be pointing to null
        assert(!this->tail);
        node_to_insert->prev = nullptr;
        node_to_insert->next = nullptr;
        this->head = node_to_insert;
        this->tail = node_to_insert;
        return;
    }

    // otherwise just insert into the beginning
    assert(!this->head->prev);
    this->insert_before(this->head, node_to_insert);
    this->head = node_to_insert;
}

template <typename Type>
void TransparentList<Type>::insert(TransparentList<Type>::NodeIterator iterator,
                                   Node<Type>* node_to_insert) noexcept {
    assert_pointer_invariants(node_to_insert);

    // if the iterator was pointing past the end of the linked list then
    // insert should insert past the last element, i.e. a push_back()
    if (iterator == NodeIterator{nullptr}) {
        this->push_back(node_to_insert);
        return;
    }

    // if not then the iterator was pointing to a valid element, insert right
    // before it
    assert(iterator.node_ptr);
    this->insert_before(iterator.node_ptr, node_to_insert);
}

template <typename Type>
void TransparentList<Type>::erase(TransparentList<Type>::NodeIterator iterator)
        noexcept {
    assert_pointer_invariants(iterator.node_ptr);

    // if this was the only node in the list then erase it and set the head
    // and tail pointers to null
    if (this->head == iterator.node_ptr) {
        assert(this->tail == iterator.node_ptr);
        this->head = nullptr;
        this->tail = nullptr;
        return;
    }
    assert(this->tail != this->head);

    // erase the current pointer
    if (iterator.node_ptr->next) {
        iterator.node_ptr->next->prev = iterator->node_ptr->prev;
    }
    if (iterator.node_ptr->prev) {
        iterator.node_ptr->prev->next = iterator.node_ptr->next;
    }
}

} // namespace sharp
