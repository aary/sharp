/**
 * @file Executor.hpp
 * @author Aaryaman Sagar
 *
 * An executor is an interface that can be used by clients to pass off units
 * of work, that will be executed as determined by the implementation of the
 * executor.  It is a basic building block for event driven systems that need
 * a systematic way to execute blocks of code
 */

#pragma once

#include <cstddef>

#include <sharp/Functional/Functional.hpp>

namespace sharp {

/**
 * @class Executor
 *
 * This class encapsulates the execution of a passed in function object.  The
 * function object is either executed inline on the current thread or on
 * another thread
 *
 * The interface to it is simple, when you want a function executed either now
 * or at some time in the immediate future (because there might be other
 * things executing currently) then you call the .add() method.  The .add()
 * method will either execute the function immediately or pass it off to
 * another thread to execute later.  The execution policy (either to execute
 * inline or in another thread) will be determined by the implementation of
 * the executor
 *
 * All implementations of executor classes will derive from this one base
 * class and then specialize the .add() member function
 */
class Executor {
public:

    /**
     * Virtual destructor for the Executor class
     */
    virtual ~Executor() {}

    /**
     * The add() function which consists of most of the logic and usage of
     * this Executor class, this function will be used to add function objects
     * to this executor, either to be executed now or later in the future
     *
     * The exact execution of the closure will be dependent on the
     * implementation of the executor, for example it can either be executed
     * inline or on another thread
     */
    virtual void add(sharp::Function<void()> closure) = 0;

    /**
     * Returns the number of function objects waiting to be executed
     *
     * This is intended for debugging/logging and other issues of the sort.
     * Any other uses are inherently prone to race conditions because other
     * threads may still be executing the closures
     */
    virtual std::size_t num_pending_closures() const;
};

} // namespace sharp

#include <sharp/Executor/InlineExecutor.hpp>
