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
     */
    MoveInto() = delete;
    MoveInto(MoveInto&&) = delete;
    MoveInto(const MoveInto&) = delete;

    /**
     * Delete the convert constructor version that would have made a copy
     */
    MoveInto(const T&) = delete;

    /**
     * The convert move constructor takes an rvalue reference to a T and moves
     * it into the private instance, forcing a move
     */
    explicit MoveInto(T&& in);

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
    const T* operator->();

private:
    T instance;
};

} // namespace sharp

#include <sharp/MoveInto/MoveInto.ipp>
