/*
 * Written by Aaryaman Sagar
 */

#pragma once

class FakeMutex {
public:
    enum class LockedState : int {LOCKED, SHARED, UNLOCKED};
    FakeMutex() : locked_state{LockedState::UNLOCKED} {}

    void lock() {
        this->locked_state = LockedState::LOCKED;
    }
    void unlock() {
        this->locked_state = LockedState::UNLOCKED;
    }
    void lock_shared() {
        this->locked_state = LockedState::SHARED;
    }

    LockedState locked_state;
};
