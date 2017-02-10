/**
 * @file TransparentList.hpp
 * @author Aaryaman Sagar
 *
 * A simple free linked list that does not offer any memory abstractions.  As
 * such the linked list class does not hook into any memory abstractions such
 * as allocators as well.  Resulting in convenience for situations where the
 * program is written right on top of an existing memory model with no
 * intermediate abstractions such as some embedded systems, kernel level
 * programming and library implementations such as malloc.
 *
 * Since it can be used for pretty much any purpose and uses pointers, the
 * linked list asserts alignment on the largest boundary of primitive types to
 * ensure correctness (invalid alignment on some architectures eg.  ARM causes
 * a fault) and maximum performance (x86 ignores unaligned accesses at the
 * cost of runtime).
 */

#pragma once

#include <utility>
#include <cassert>
#include <iterator>
#include <initializer_list>

namespace sharp {

/**
 * The node class, elements are stored in here, these should be pushed onto
 * the linked list
 */
template <typename Type>
class Node {
public:
    /**
     * In place construction with variadic arguments that can be forwarded to
     * the constructor of type Datum
     */
    template <typename Args>
    Node(std::in_place_t, Args&&... args)
            : datum{std::forward<Args>(args)...} {}

    /**
     * In place construction with variadic arguments and an initializer list
     * that can be forwarded to the constructor of type Datum.  This is
     * required because initializer lists are not deducible via templates.
     */
    template <typename U, typename Args>
    Node(std::in_place_t, std::initializer_list<U> ilist, Args&&... args)
            : datum{std::move(ilist), std::forward<Args>(args)...} {}

    /**
     * The datum, this is made public so that the user can interact with it as
     * they wish
     */
    Type datum;

    /**
     * Make the list class a friend so that it can access the pointers in the
     * struct
     */
    friend class TransparentList<Type>;

private:

    /**
     * the previous next pointers and the data item
     */
    Node<Type>* prev;
    Node<Type>* next;
};

template <typename Type>
class TransparentList {
public:

    /**
     * An iterator class for generality and convenience
     */
    class NodeIterator {
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
        using value_type = Node<Type>;
        using pointer = Node<Type>*;
        using reference = Node<Type>&;
        using iterator_category = std::forward_iterator_tag;

        /**
         * Dereferenece operator
         */
        auto& operator*() noexcept;

        /**
         * Chain operator->, returns a pointer to the held node.  The compiler
         * chains operator-> calls on everything returned by the call itself,
         * so calling operator-> on an instance of a NodeIterator is
         * equivalent to calling operator-> on a Node<Type>* object
         */
        Node<Type>* operator->() noexcept;

        /**
         * Preincrement operator to advance the node pointer to the next spot
         */
        NodeIterator& operator++() noexcept;

        /**
         * Equality operators to determine whether two iterators are pointing to
         * the same node
         */
        bool operator==(const Node<Type>& other) const noexcept;
        bool operator!=(const Node<Type>& other) const noexcept;

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
        NodeIterator(Node<Type>* node_in) noexcept : node{node_in};

        /**
         * A pointer to the node that the iterator refers to
         */
        Node<Type>* node_ptr;
    };

    /**
     * A default constructor that initializes the linked list to point to
     * nothing at all
     */
    TransparentList() noexcept;

    /**
     * Method to push back and front a node to the linked list, the node
     * should be created and it's scope should not be an issue here
     */
    void push_back(Node<Type>* node_to_insert) noexcept;
    void push_front(Node<Type>* node_to_insert) noexcept;

    /**
     * Methods to pop back from a linked list and pop front from a linked list
     */
    void pop_back() noexcept;
    void pop_front() noexcept;

    /**
     * Method to insert a given node right before the element pointed to by the
     * iterator
     */
    void insert(NodeIterator iterator, Node<Type>* node_to_insert)
        noexcept;

    /**
     * Given an iterator to an element in the linked list, remove it from the
     * list
     */
    void erase(NodeIterator iterator) noexcept;

    /**
     * Return an iterator to the beginning of the linked list
     */
    auto begin() noexcept;

    /**
     * Return an iterator past the end of the linked list.  This iterator
     * should be pointing to null internally
     */
    auto end() noexcept;

private:

    /**
     * Insert right after a node, this does not check for validity
     */
    void insert_after(Node<Type>* to_insert_after, Node<Type>* to_insert)
        noexcept;

    /**
     * Insert right before a node, this does not check for validity or
     * anything
     */
    void insert_before(Node<Type>* to_insert_before, Node<Type>* to_insert)
        noexcept;

    /**
     * The head and tail pointers are the only bookkeeping in this list
     */
    Node<Type>* head;
    Node<Type>* tail;
};

} // namespace sharp

#include <sharp/TransparentList/TransparentList.ipp>
