/**
 * @file MoveInto.hpp
 * @author Aaryaman Sagar
 */

#pragma once

namespace sharp {

template <typename T>
class MoveInto {
public:

    /**
     * The default constructor and any constructor that constructs this from
     * another instance of MoveInto is deleted, an instance of MoveInto can
     * only be constructed by moving an object of type T
     *
     * Also delete the copy convert constructor, copies should not be passed
     * here
     */
    MoveInto() = delete;
    MoveInto(const MoveInto&) = delete;
    MoveInto(const T&) = delete;

    /**
     * The move constructor needs to be defaulted in C++14 because prvalue
     * elision is not mandatory and code will not compile if an rvalue is used
     * to construct a MoveInto instance in a function parameter
     */
    MoveInto(MoveInto&&) = default;

    /**
     * The convert move constructor takes an rvalue reference to a T and moves
     * it into the private instance, forcing a move
     */
    MoveInto(T&& in);

    /**
     * The destructor is defaulted
     */
    ~MoveInto() = default;

    /**
     * Pointer like methods to use this like a pointer
     */
    T& operator*() &;
    const T& operator*() const &;
    T&& operator*() &&;
    const T&& operator*() const &&;
    T* operator->();
    const T* operator->() const;

private:
    T instance;
};

} // namespace sharp

#include <sharp/MoveInto/MoveInto.ipp>
