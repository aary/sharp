/*
 * Written by Aaryaman Sagar
 */

#pragma once

class FakeMutex {
public:
    enum class LockState : int {LOCKED, SHARED, UNLOCKED};
    FakeMutex() : lock_state{LockState::UNLOCKED} {}

    virtual void lock() {
        this->lock_state = LockState::LOCKED;
    }
    virtual void unlock() {
        this->lock_state = LockState::UNLOCKED;
    }
    virtual void lock_shared() {
        this->lock_state = LockState::SHARED;
    }

    LockState lock_state;
};
