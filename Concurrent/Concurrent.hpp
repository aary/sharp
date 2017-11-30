/**
 * @file Concurrent.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This module contains an abstraction that serves better than mutexes for
 * mutual exclusion and monitors for synchronization, with a uniform easy to
 * understand API this eliminates some of the drawbacks of mutexes and
 * monitors
 *
 *  * Originally written for EECS 482 @ The University of Michigan *
 *
 * This was originally named ThreadSafeData, but the name concurrent works
 * better and is shorter :)
 *
 * See the class description documentation for simple use case examples.
 */

#pragma once

#include <sharp/Functional/Functional.hpp>
#include <sharp/Concurrent/Concurrent.pre.hpp>

#include <sharp/Tags/Tags.hpp>
#include <sharp/Portability/cpp17.hpp>

#include <condition_variable>
#include <utility>
#include <type_traits>
#include <mutex>
#include <unordered_map>

namespace sharp {

/**
 * @class Concurrent
 *
 * This class can be used as a simple wrapper to disallow non locked access to
 * a data item with an RAII solution to achieving simple critical sections,
 * this was one of the motivating usecases for the class
 *
 *      sharp::Concurrent<std::vector> vec;
 *      auto& ele = vec.synchronized([](auto& vec) {
 *          return v.size();
 *      });
 *
 * The access to the vector above is made under the protection of a mutex and
 * is therefore safe
 *
 * There is also a locked proxy interface which offers another alternative to
 * simple critical sections
 *
 *      auto vec_lock = vec.lock();
 *      cout << vec_lock->size() << endl;
 *      vec_lock->push_back(Something{});
 *      vec_lock.unlock();
 *
 * Here the vec.lock() method returns a proxy object that acquires the
 * internal lock on construction and releases the lock either on destruction
 * or on invocation of the unlock() function as shown above.  This helps the
 * user write exception safe concurrent code, and not forget to accidentally
 * unlock the mutex when an exception is propagated from say the push_back
 *
 * Furhter this class can be used to get automated shared locking integrated
 * with the C++ type system.  Conceptually a shared lock (or a reader writer
 * lock) is an optimization over a traditional mutex that allows multiple
 * readers to read the data item concurrently.  This helps systems achieve
 * high throughput in high read scenarios.  Read-only locking or shared
 * locking occurs conceptually on data that is not meant to be modified, in
 * C++ this concept is represented as a const modifier on an object.  The
 * presence of the const modifier should convey read only usage of the
 * underlying data item.
 *
 * This class gives the user the benefits of shared locking automatically,
 * when the C++ type system is utilized correctly when a const
 * sharp::Concurrent object is locked, the library automatically acquires a
 * shared lock on the underlying mutex if it can.  Providing an abstraction
 * that opaquely operates to provide high concurrent throughput for read only
 * threads
 *
 *      void read(const sharp::Concurrent<std::vector>& vec) {
 *          // this acquies a shared lock on the underlying vector
 *          auto vec_lock = vec.lock();
 *          cout << vec_lock.size() << endl;
 *      }
 *
 * sharp::Concurrent tries to provide a conditional critical section API
 * (inspired by Google's Abeil Mutex class, see goo.gl/JhhGFp) to the user as
 * well, this tries to eliminate the drawbacks associated with condition
 * variables
 *
 *      // thread 1
 *      auto lock = sharp::as_const(data).lock();
 *      lock.wait([](auto& data) {
 *          return data.is_ready();
 *      });
 *      cout << lock->get() << endl;
 *
 *      // thread 2
 *      auto lock = data.lock();
 *      lock->set_value(3);
 *      lock.unlock();
 *
 * Here when the writer thread is done, the reader will be woken up.  Simple.
 * No complicated signaling, broadcasting, no forgetting to signal, no
 * nothing.  When the write is done the reader is woken up.
 *
 * Further when the data item provides an is_ready() method or ADL defined
 * free function, the reader wait looks even simpler, the waiter is woken up
 * when the data item returns true from its is_ready() function
 *
 *      // thread 1
 *      auto lock = sharp::as_const(data).lock();
 *      lock.wait();
 *      cout << lock->get() << endl;
 *
 *      // thread 2
 *      auto lock = data.lock();
 *      lock->set_value(3);
 *      lock.unlock();
 *
 * Objects of the class are both copyable and movable, however neither of those
 * operations and their variants are noexcept in all cases.  This is because
 * the internal mutex can throw, and when the mutex class can throw thos
 * operations cannot be labelled noexcept.  But if the mutex class's lock()
 * method is noexecpt, then this class switches to provide the noexcept
 * guarantee wherever it can
 */
template <typename Type,
          typename Mutex = std::mutex,
          typename Cv = typename concurrent_detail::GetCv<Mutex>::type>
class Concurrent : public concurrent_detail::Conditions<
            Mutex, Cv, sharp::Function<bool(const Type&)>> {
public:

    /**
     * Customary typedef for the value type stored here
     */
    using value_type = Type;
    using mutex_type = Mutex;
    using cv_type = Cv;

private:
    /**
     * @class LockProxy
     *
     * This is the proxy class that is returned by the Concurrent::lock()
     * method, consult the documentation for Concurrent for more details
     *
     * Instances of this class act as smart pointer types to the underlying
     * object, i.e.  they overload the arrow (->) and dereference (*)
     * operators; which both return pointers and references to the underlying
     * object
     *
     * These are RAII lock objects, they acquire the underlying mutex on
     * construction and release the mutex on destruction.  They also support
     * manual unlock() methods which can release the lock before destruction.
     * In the event of lock releasing before destruction these make their
     * internal object pointers point to null, so any future attempts to
     * access the underlying object would either result in a crash or lead to
     * undefined behavior, which tools like ASAN should easily detect
     *
     * Note that because this class is private to the outside scope you have
     * to use auto to construct a proxy class, and you don't have access to
     * the type of the class, so you cannot make it a member variable, pass it
     * to a function without going through extreme hardships
     */
    template <typename ConcurrentType, typename>
    class LockProxy {
    public:

        /**
         * Declare the value type as being a const T if the concurrent type
         * itself is const and non const otherwise
         */
        using value_type = std::conditional_t<
            std::is_const<ConcurrentType>::value,
                const typename std::decay_t<ConcurrentType>::value_type,
                typename std::decay_t<ConcurrentType>::value_type>;

        /**
         * Can be move constructed to allow convenient construction via auto in
         * C++11 and C++14
         *
         *      auto lock = vec.lock();
         *
         * Although this will not really cause a move in most most cases this
         * will still not compile before C++17 (with mandatory prvalue
         * elision).  So the move constructor is needed
         */
        LockProxy(LockProxy&&);

        /**
         * The unlock function puts the proxy into an irrecoverable null
         * state.  Since there is no lock() or assignment operator provided,
         * once a lock proxy is null it cannot be used afterwards
         */
        void unlock() noexcept;

        /**
         * Destructor calls unlock() and unlocks the mutex
         */
        ~LockProxy();

        /**
         * Pointer like methods for accessing the underlying data object,
         * these will return const items based on how the interface is
         * instantiated in the implementation
         */
        value_type* operator->();
        value_type& operator*();

        /**
         * Waits on the condition passed until some other lock proxy unlocks
         * and this condition becomes true, in which case the call to this
         * function returns.  Blocks otherwise
         *
         * This method can only be called when the lock proxy is in a valid
         * non null state, when called from a null state, the program exhibits
         * undefined behavior
         *
         * If multiple threads are waiting on the same condition it is useful
         * to pass the same function pointer or lambda to this function.  In
         * that case the concurrent object internally only allocates one
         * condition variable for all those conditions.  Whereas if different
         * threads want to wait on different conditions, different conditions
         * should be passed
         *
         * If the wrapped class has a const is_ready() function, the condition
         * itself can be skipped enitrely and left empty, the lock proxy will
         * check for the is_ready function on some other unlock and wake up
         * the current thread if the method is satisfied
         *
         * Note that passing lamdbas with no capture is completely ok here
         * since they support conversions to function pointers.  In fact it is
         * recommended for readability that lambdas are passed in here for
         * short conditions
         */
        template <typename Condition>
        void wait(Condition condition);

        /**
         * Friend the outer concurrent class, it is the only one that can
         * construct objects of type LockProxy
         */
        friend class Concurrent;
        template <typename, typename, typename, bool>
        friend class concurrent_detail::ConditionsLockWrap;

    private:
        /**
         * Constructor locks the mutex
         */
        explicit LockProxy(ConcurrentType&);

        /**
         * Constructor that does not call lock() on the mutex but calls unlock
         * on destruction
         */
        LockProxy(std::adopt_lock_t, ConcurrentType&);

        /**
         * The assignment operators and the copy constructor are deleted
         * because those dont make the most sense here and dont represent
         * logical operations.  What does it mean to copy a lock proxy?
         */
        LockProxy(const LockProxy&) = delete;
        LockProxy& operator=(const LockProxy&) = delete;
        LockProxy& operator=(LockProxy&&) = delete;

        /**
         * A pointer is helpful in detecting use-after-unlock cases where the
         * program should abort.  The other solution is the manually check on
         * every access whether the lock is acquired or not, that is slow and
         * therefore the solution used here is to make a lock proxy a one use
         * only construct
         *
         * When the lock is unlocked the instance_ptr points to null, and
         * therefore accesses will cause a null dereference which is easy to
         * catch with tools like ASAN, valgrind or gdb
         */
        ConcurrentType* instance_ptr{nullptr};
    };

public:
    /**
     * Accepts a callable as an argument and executes that on an internal lock
     *
     * The lock is held according to the constness of the object, if
     * the calling object is const then the implementation tries to acquire a
     * shared lock if the mutex interface supports it.  If the mutex doesn't
     * support a shared lock then an exclusive lock is acquried
     *
     * Any exceptions propagating from the passed invocable cause the mutex to
     * get unlocked and the exception propagated, if an exception is thrown
     * from the lock function of the mutex, that too is propagated to the
     * user.  If however the unlock function throws, std::terminate() is
     * called
     *
     * The return value of the function is forwarded as the return value of
     * this function
     */
    template <typename F>
    decltype(auto) synchronized(F&&);

    /**
     * Const version of the same function that locks the object via acquiring
     * a read lock if the type of mutex passed in supports shared locking
     */
    template <typename F>
    decltype(auto) synchronized(F&&) const;

    /**
     * Returns an RAII proxy object that locks the inner data object on
     * construction and unlocks it on destruction
     *
     * The proxy objects act as smart pointers to the underlying data item and
     * overload -> and * to allow easy access
     *
     * Note that this method has been overloaded on the basis of whether the
     * object is const or not.  If the Concurrent<> object is non const then
     * this function is called, and this method tries to acquire a shared lock
     * on the underlying mutex.  If a shared lock API is not supported by the
     * mutex, then this function acquires a unique lock
     */
    auto /* LockProxy<> */ lock();

    /**
     * A const version of the same lock above.  This helps to automate the
     * process of acquiring a read lock when the lock type provided is a
     * readers-writer lock or some form of shared lock.
     */
    auto /* LockProxy<> */ lock() const;

    /**
     * The usual constructors for the class.  This is set to the default
     * constructor for the class, the mutex and datum are default constructed
     */
    Concurrent() = default;

    /**
     * Copy constructor.
     *
     * Note that the internal mutex is held here and therefore the
     * constructors dont have the option of being noexcept.
     *
     * Also note that this *does not* do any copying from the mutex.  The
     * mutex is left in its default constructed state.  And the object itself
     * is copied while holding a mutex on the rhs
     */
    Concurrent(const Concurrent& other);

    /**
     * Move constructor like the copy constructor acquires a lock on the mutex
     * of the right hand side and moves the data object into the current
     * concurrent object
     *
     * Also note that this is not marked noexcept, this means when
     * synchronized objects are in a container like vector which requires
     * moving to be noexcept for a strong exception safety guarantee, the
     * objects will be copied instead of being moved
     */
    Concurrent(Concurrent&& other);

    /**
     * Constructs the object from either a copy of the templated type or by
     * moving it into the internal storage
     */
    explicit Concurrent(const Type& instance);
    explicit Concurrent(Type&& instance);

    /**
     * This constructor is present to allow in place construction of the
     * internal object.  All arguments are forwarded to the constructor of the
     * data object.
     *
     * To use this form of construction, select dispatch with a std::in_place
     * tag.  For example
     *
     *      auto thing = Concurrent<Something>{std::in_place, one, two, three};
     *
     * Note that the internal mutex is not held here and therefore the
     * constructors have the option of being noexcept
     */
    template <typename... Args>
    Concurrent(std::in_place_t, Args&&...args);
    template <typename U, typename... Args>
    Concurrent(std::in_place_t, std::initializer_list<U> il, Args&&... args);

    /**
     * Copy assignment operator
     *
     * Note that this is not declared noexcept because the locks have to be
     * held when assigning a locked object.
     */
    Concurrent& operator=(const Concurrent&);

    /**
     * Move assingment does the smae thing as copy assignment.  Holds the
     * mutex in a predetermined order and then moves the object from other
     * into this
     *
     * Also note that this is not noexcept because the internal mutex can
     * throw
     */
    Concurrent& operator=(Concurrent&& other);

    /**
     * Friend the proxy class and the bookkeeping class, they should be able to
     * access internals of this class
     */
    template <typename, typename>
    friend class LockProxy;
    template <typename, typename, typename, bool>
    friend class concurrent_detail::ConditionsLockWrap;
    template <typename... Args>
    friend std::tuple<decltype(std::declval<Args>().lock())...> lock(Args&...);

private:

    /**
     * Used in constructors to perform some action before and after the
     * construction of the datum.  In this case this is used in constructors
     * to directly initialize the datum while holding the lock before and
     * releasing it after the construction is done
     *
     * The delegate constructor tag is used for clarity
     *
     * Action represents a struct that performs some action on construction
     * and releases it on destruction
     */
    template <typename Action, typename... Args>
    Concurrent(sharp::delegate_constructor::tag_t, Action, Args&&...);

    /**
     * Implementation of the move and copy constructors for the class
     */
    template <typename OtherConcurrent>
    Concurrent(sharp::implementation::tag_t, OtherConcurrent&& other);

    /**
     * Return a lock proxy that does not call the lock function on
     * construction
     */
    auto /* LockProxy<> */ lock(std::adopt_lock_t);
    auto /* LockProxy<> */ lock(std::adopt_lock_t) const;

    /**
     * The data object that is to be locked and the internal mutex used to
     * synchronize
     */
    Type datum;
    mutable Mutex mtx;

    /**
     * Friend for testing
     */
    friend class ConcurrentTests;
};

/**
 * @function lock
 *
 * A deadlock avoidance function that can lock a bunch of concurrent objects
 * in the same globally consistent order and return a tuple of lock proxies
 * that can be used to work with those objects
 *
 *      auto [one, two, three] = sharp::lock(vec, deq, mp);
 *
 * Note that this is almost the same as std::lock
 */
template <typename... Args>
std::tuple<decltype(std::declval<Args>().lock())...> lock(Args&... args);

} // namespace sharp

#include <sharp/Concurrent/Concurrent.ipp>
