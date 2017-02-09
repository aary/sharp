#include <cassert>
#include <iterator>
#include <cstdint>

#include "TransparentList.hpp"

namespace eecs281 {

template <typename Type>
static void assert_pointer_invariants(Type* pointer) {
    assert(pointer);
    assert(!(reinterpret_cast<uintptr_t>(pointer) % alignof(std::max_align_t)));
}

/**
 * Implementations of the iterator class methods
 */
/**
 * The dereference operator for the node iterator class
 */
template <typename Type>
auto& TransparentList<Type>::NodeIterator::operator*() {
    return *this->node_ptr;
}

template <typename Type>
Node<Type>* TransparentList<Type>::NodeIterator::operator->() {
    return this->node_ptr;
}

/**
 * Preincrement operator for the iterator to elements of the doubly linked list
 */
template <typename Type>
TransparentList<Type>::NodeIterator&
TransparentList<Type>::NodeIterator::operator++() {
    this->node_ptr = this->node_ptr->next;
    return *this;
}

/**
 * Comparison operators
 */
template <typename Type>
bool TransparentList<Type>::NodeIterator::operator==(
        const TransparentList<Type>::NodeIterator& other) const noexcept {
    return this->node_ptr == other.node_ptr;
}
template <typename Type>
bool TransparentList<Type>::NodeIterator::operator!=(
        const TransparentList<Type>::NodeIterator& other) const noexcept {
    return this->node_ptr != other.node_ptr;
}

/**
 * Implementations for the linked list class methods
 */
template <typename Type>
TransparentList<Type>::TransparentList() noexcept
        : head{nullptr}, tail{nullptr} {}

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
    }

    // otherwise just insert into the linked list
    assert(!this->tail->next);
    node_to_insert->next = nullptr;
    node_to_insert->prev = this->tail;
    this->tail->next = node_to_insert;
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
    }

    // otherwise just insert into the beginning
    assert(!this->head->prev);
    node_to_insert->prev = nullptr;
    node_to_insert->next = this->head;
    this->head->prev = node_to_insert;
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
    }

    // if not then the iterator was pointing to a valid element, insert right
    // before it
    node_to_insert->next = iterator.node_ptr;
    node_to_insert->prev = iterator.node_ptr->prev;
    iterator.node_ptr->prev = node_to_insert;
}

template <typename Type>
void TransparentList<Type>::erase(TransparentList<Type>::NodeIterator iterator)
        noexcept {
    assert_pointer_invariants(iterator.node_ptr);
}

} // namespace eecs281
