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

#include <sharp/Tags/Tags.hpp>

#include <utility>
#include <type_traits>
#include <mutex>

namespace sharp {

/**
 * @class Concurrent
 *
 * This class can be used as a simple wrapper to disallow non locked access to
 * a data item with an RAII solution to achieving simple critical sections,
 * this was one of the motivating usecases for the class
 *
 *      sharp::Concurrent<std::vector> vec;
 *      auto& ele = vec.atomic([](auto& vec) { return v.size(); });
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
 * Note that this class is not moveable.  If you want move semantics, because
 * for example you are including objects of this class in a map, then just put
 * it in a unique_ptr and put that pointer in the map.
 */
template <typename Type, typename Mutex = std::mutex>
class Concurrent {
public:

    /**
     * Synchronized function that accepts a function.  The requirements for a
     * function are enforced by SFINAE.
     *
     * The return value is perfectly forwarded to the called by means of the
     * decltype(auto) return type.
     *
     * Note that the single rare case where this does not happen in the case
     * of `decltype((x))` where the `(x)` neccesitates decltype to return a
     * reference type has been avoided here in the implementation.
     *
     * @param func A function that accepts an argument of type Type&, this
     *             object of type Type& will be the unwrapped element and not
     *             the Concurrent element.
     * @return returns the exact object that is returned by the function when
     *         executed on the internal object
     */
    template <typename Func>
    decltype(auto) atomic(Func&&)
        /* -> decltype(std::declval<Func>(std::declval<Type>())) */;

    /* Const version of the same function that locks the object via acquiring
     * a read lock if the type of mutex passed in supports shared locking
     *
     * @param func A function that accepts a reference to an object of type
     *             Type
     * @return returns the exact object that is returned by the function when
     *         executed on the internal object
     */
    template <typename Func>
    decltype(auto) atomic(Func&&) const
        /* -> decltype(std::declval<Func>()(std::declval<Type>())) */;

    /**
     * Forward declarations of lightweight proxy types that are used to
     * interact with the underlying object.
     */
    class UniqueLockedProxy;
    class ConstUniqueLockedProxy;

    /**
     * Returns a proxy object that locks the inner data object on construction
     * and unlocks it on destruction.
     *
     * This should be updated to also return a proxy object that behaves as a
     * reference when the operator.() becomes standard.
     *
     * Note that this method has been overloaded on the basis of whether the
     * object is const or not.  When the Concurrent object is const in context
     * then this function is called and when the object is mutable in context
     * then the version above is called
     */
    UniqueLockedProxy lock();

    /**
     * A const version of the same lock above.  This helps to automate the
     * process of acquiring a read lock when the lock type provided is a
     * readers-writer lock or some form of shared lock.
     *
     * Note that this method has been overloaded on the basis of whether the
     * object is const or not.  When the Concurrent object is const in context
     * then this function is called and when the object is mutable in context
     * then the version above is called
     */
    ConstUniqueLockedProxy lock() const;

    /**
     * The usual constructors for the class.  This is set to the default
     * constructor for the class.
     */
    Concurrent() = default;

    /**
     * Copy constructor.
     *
     * Note that the internal mutex is held here and therefore the
     * constructors dont have the option of being noexcept.
     *
     * Also note that this *does not* do any copying from the mutex.  The
     * mutex is left in its default constructed state.
     *
     * @param other the other locked data object that is to be copied
     */
    Concurrent(const Concurrent&);

    /**
     * Move constructor has been deleted because it probably should not be
     * used.  The inner mutex is not going to be moved.  Wrapping it in a
     * unique_ptr would cause loss of performance and is unacceptable here
     */
    Concurrent(Concurrent&&) = delete;

    /**
     * This constructor is present to allow simulation of an aggregate type by
     * the Concurrent object.  All arguments are forwarded to the constructor
     * of the data object.
     *
     * To use this form of construction, select dispatch with a
     * sharp::emplace_construct_t object.  For example
     *
     *  Concurrent<Class> locked {
     *      std::emplace_construct_t, one, two, three};
     *
     * Note that the internal mutex is not held here and therefore the
     * constructors have the option of being noexcept
     *
     * @param emplace_construct_t a tag that is used to clarify that the
     * arguments are going to be forwarded to the constructor of the
     * underlying object
     * @param args... arguments to forward to the constructor of the stored
     * object
     */
    template <typename... Args>
    explicit Concurrent(sharp::emplace_construct::tag_t, Args&&...args)
        noexcept(noexcept(Type(std::forward<Args>(args)...)));

    /**
     * Copy assignment operator
     *
     * Note that this is not declared noexcept because the locks have to be
     * held when assigning a locked object.
     */
    Concurrent& operator=(const Concurrent&) /* noexcept */;

    /**
     * Deleted the move assignment operator becauase I saw no reason to
     * include it.  This should not be moved across threads?  If there is a
     * dire need to move this around, it can be dumped in a unique_ptr and
     * then moved
     */
    Concurrent& operator=(Concurrent&& other) = delete;

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
    explicit Concurrent(sharp::delegate_constructor::tag_t, Action, Args&&...);

    /**
     * Implementation of the copy constructor for the class
     */
    explicit Concurrent(sharp::implementation::tag_t, const Concurrent&);

    /**
     * The data object that is to be locked
     */
    Type datum;

    /**
     * The internal mutex
     */
    mutable Mutex mtx;

    /* Friend for testing */
    friend class ConcurrentTests;
};

} // namespace sharp

#include <sharp/Concurrent/Concurrent.ipp>
