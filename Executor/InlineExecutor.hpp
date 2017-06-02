/**
 * @file InlineExecutor.hpp
 * @author Aaryaman Sagar
 *
 * The simplest possible implementation of the Executor interface, all
 * callbacks are executed immediately on calling the .add() function on the
 * current thread
 */

#pragma once

#include <cstddef>

#include <sharp/Executor/Executor.hpp>
#include <sharp/Functional/Functional.hpp>

namespace sharp {

class InlineExecutor : public Executor {
public:

    /**
     * Destructor does nothing as there is no state to maintain here, all
     * functions are executed immediately
     */
    ~InlineExecutor() override {}

    /**
     * Do the thing
     */
    void add(sharp::Function<void()> closure) override {
        closure();
    }

    /**
     * At all times there are no pending closures in this executor, as all of
     * them are executed immediately
     */
    std::size_t num_pending_closures() const override {
        return 0;
    }

    /**
     * Returns the thread singleton inline executor instance
     */
    static Executor* get() {
        static InlineExecutor executor;
        return &executor;
    }

};

} // namespace sharp
