/*
 * Written by Aaryaman Sagar
 */

#pragma once

#include <cassert>

class FakeMutex {
public:
    enum class LockState : int {LOCKED, SHARED, UNLOCKED};
    FakeMutex() : lock_state{LockState::UNLOCKED} {}

    virtual void lock() {
        assert(this->lock_state == LockState::UNLOCKED);
        this->lock_state = LockState::LOCKED;
    }
    virtual void unlock() {
        assert(this->lock_state == LockState::LOCKED);
        this->lock_state = LockState::UNLOCKED;
    }
    virtual void lock_shared() {
        assert(this->lock_state == LockState::UNLOCKED);
        this->lock_state = LockState::SHARED;
    }
    virtual void unlock_shared() {
        assert(this->lock_state == LockState::SHARED);
        this->lock_state = LockState::UNLOCKED;
    }

    LockState lock_state;
};
