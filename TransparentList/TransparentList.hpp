/**
 * @file TransparentList.hpp
 * @author Aaryaman Sagar
 *
 * A simple free linked list that does not offer any memory abstractions.  As
 * such the linked list class does not hook into any memory abstractions such
 * as allocators as well.  Resulting in convenience for situations where the
 * program is written right on top of an existing memory model with no
 * intermediate abstractions such as some embedded systems, kernel level
 * programming and library implementations such as malloc.  Good C++
 * abstractions are usually not available in these cases, and the result is
 * often bad code which could be more idiomatic
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

#include <sharp/Tags/Tags.hpp>

namespace sharp {

/**
 * Forward declaration for the linked list class
 */
template <typename Type>
class TransparentList;

/**
 * The node class, elements are stored in here, these should be pushed onto
 * the linked list
 *
 * The node class is aligned to the maximum boundary as set by the system,
 * this is done to ensure compatibility with low level interfaces and use
 * cases on all systems
 */
template <typename Type>
class alignas(alignof(std::max_align_t)) TransparentNode {
public:

    /**
     * In place construction with variadic arguments that can be forwarded to
     * the constructor of type Datum
     */
    template <typename... Args>
    TransparentNode(sharp::emplace_construct::tag_t, Args&&... args)
            : datum{std::forward<Args>(args)...} {}

    /**
     * In place construction with variadic arguments and an initializer list
     * that can be forwarded to the constructor of type Datum.  This is
     * required because initializer lists are not deducible via templates.
     */
    template <typename U, typename... Args>
    TransparentNode(sharp::emplace_construct::tag_t,
                    std::initializer_list<U> ilist,
                    Args&&... args)
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
    TransparentNode<Type>* prev;
    TransparentNode<Type>* next;
};

template <typename Type>
class TransparentList {
public:

    /**
     * An iterator class for generality and convenience
     */
    class NodeIterator;

    /**
     * A default constructor that initializes the linked list to point to
     * nothing at all
     */
    TransparentList() noexcept;

    /**
     * Method to push back and front a node to the linked list, the node
     * should be created and it's scope should not be an issue here
     */
    void push_back(TransparentNode<Type>* node_to_insert) noexcept;
    void push_front(TransparentNode<Type>* node_to_insert) noexcept;

    /**
     * Methods to pop back from a linked list and pop front from a linked list
     */
    void pop_back() noexcept;
    void pop_front() noexcept;

    /**
     * Method to insert a given node right before the element pointed to by the
     * iterator
     */
    void insert(NodeIterator iterator, TransparentNode<Type>* node_to_insert)
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

    /**
     * Return an iterator that points to the node passed in
     */
    static auto make_iterator(TransparentNode<Type>* node);

private:

    /**
     * Insert right after a node, this does not check for validity
     */
    void insert_after(TransparentNode<Type>* to_insert_after,
                      TransparentNode<Type>* to_insert) noexcept;

    /**
     * Insert right before a node, this does not check for validity or
     * anything
     */
    void insert_before(TransparentNode<Type>* to_insert_before,
                       TransparentNode<Type>* to_insert) noexcept;

    /**
     * The head and tail pointers are the only bookkeeping in this list
     */
    TransparentNode<Type>* head;
    TransparentNode<Type>* tail;
};

} // namespace sharp

#include <sharp/TransparentList/TransparentList.ipp>
