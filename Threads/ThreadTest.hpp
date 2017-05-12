/**
 * @file ThreadTest
 * @author Aaryaman Sagar
 *
 * Contains a simple threading abstraction that can be used to write
 * multithreaded test cases
 *
 * Write test cases like the following
 *
 *  int main() {
 *      auto th_one = std::thread{[]() {
 *          auto mark = ThreadTest::mark(0);
 *          // do something ...
 *      }};
 *
 *      auto th_two = std::thread{[]() {
 *          auto mark = ThreadTest::mark(2);
 *          // do something else ...
 *      }};
 *
 *      auto mark = ThreadTest::mark(1);
 *      // something else...
 *      mark.release();
 *
 *      th_one.join();
 *      th_two.join();
 *  }
 */

#pragma once

namespace sharp {

namespace detail {

    /**
     * Forward declaration of the RAII class that will be used to release a
     * mark and wait for a sequence of execution
     */
    class ThreadTestRaii;

} // namespace detail

/**
 * @class ThreadTest
 *
 * A simple namespace like wrapper around two functions that are the essence
 * of this module, mark() and reset(), their uses and examples are documented
 * in the comment block right above their declarations
 */
class ThreadTest {
public:

    /**
     * The basic function that is to be used by tests, this function blocks
     * until the mark has been satisfied
     *
     * For example you can easily set a sequence of execution for two threads
     * as in the following example
     *
     *  int main() {
     *      auto th_one = std::thread{[]() {
     *          auto mark = ThreadTest::mark(0);
     *          // do something ...
     *      }};
     *
     *      auto th_two = std::thread{[]() {
     *          auto mark = ThreadTest::mark(2);
     *          // do something else ...
     *      }};
     *
     *      ThreadTest::mark(1);
     *      // something else...
     *
     *      th_one.join();
     *      th_two.join();
     *
     * This allows for somewhat less painful testing when it comes to making
     * multithreaded tests
     */
    static detail::ThreadTestRaii mark(int value);

    /**
     * Resets the mark values back to 0 so that the first mark that will be
     * a future mark(0) will not block, and if a mark(0) is already waiting
     * this function will unblock that
     */
    static void reset();
};

namespace detail {

    class ThreadTestRaii {
    public:

        /**
         * Destructor makes releases the current lock and notifies the next
         * mark to go
         */
        ~ThreadTestRaii();

        /**
         * Move constructor takes over the current marked scope
         */
        ThreadTestRaii(ThreadTestRaii&&);

        /**
         * Releases the current block, use this function when you don't want
         * to wait for the destructor to make the next mark available to be
         * executed
         */
        void release();

        /**
         * Make friends with the ThreadTest class
         */
        friend class sharp::ThreadTest;

    private:

        /**
         * The value that the current block holds
         */
        int value;

        /**
         * Whether the destructor should release the current marked value or
         * not
         */
        bool should_release{true};

        /**
         * Private constructor only to be called from the mark() factory
         * function
         */
        ThreadTestRaii(int value_in);
    };

} // namespace detail

} // namespace sharp
