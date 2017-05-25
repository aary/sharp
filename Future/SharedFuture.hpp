/**
 * @file SharedFuture.hpp
 * @author Aaryaman Sagar
 *
 * A shared future is the same conceptually as a future, but instead of a
 * get() operation moving the value out of the future, a shared future's get()
 * operation copies the value out of the shared state of the future to the
 * caller
 *
 * As a result this has slightly different implications on the classes that
 * can be used to instantiate the future, unlike in a future
 *
 * Any missing documentation is the same as the corresponding documentation
 * for future
 */

#pragma once

#include <memory>

#include <sharp/Future/Future.hpp>

namespace sharp {

template <typename Type>
class SharedFuture : public detail::ComposableFuture<SharedFuture<Type>> {
public:

    /**
     * Traits that I thought might be useful
     */
    using value_type = Type;

    /**
     * Constructors, the semantics are almost the same as sharp::Future,
     * except for the copy constructors, in which the shared state is copied
     */
    SharedFuture() noexcept;
    SharedFuture(SharedFuture&&) noexcept;
    SharedFuture(const SharedFuture&);
    SharedFuture(Future<SharedFuture<Type>>&&);
    SharedFuture(Future<Type>&&) noexcept;

    /**
     * Assignment operators
     */
    SharedFuture& operator=(const SharedFuture&) = default;
    SharedFuture& operator=(SharedFuture&&) noexcept = default;

    /**
     * Returns a const reference to the value in the shared state, if there is
     * no value then the function blocks, if there is no shared state then an
     * exception is thrown
     */
    const Type& get() const;

    /**
     * The same as sharp::Future
     */
    bool valid() const noexcept;
    void wait() const;
    bool is_ready() const noexcept;

    /**
     * The same as sharp::Future::then but instead the function/functor passed
     * in should accept a shared_future by value
     */
    template <typename Func,
              typename detail::EnableIfDoesNotReturnFutureShared<Func, Type>*
                  = nullptr>
    auto then(Func&& func) -> Future<decltype(func(*this))>;
    template <typename Func,
              typename detail::EnableIfReturnsFutureShared<Func, Type>*
                  = nullptr>
    auto then(Func&& func) -> decltype(func(*this));

    /**
     * Make friends with the promise class
     */
    template <typename T>
    friend class sharp::Promise;

    /**
     * Make friends with all instantiations of the future class, this is
     * needed because there are a lot of methods here which rely on futures
     * that have been instantiated differently from the current instantiation
     * of the future class.  And it is nice to be able to call private methods
     * on these other instantiations as well
     */
    template <typename T>
    friend class sharp::SharedFuture;

    /**
     * Make friends with the SharedFuture class
     */
    template <typename T>
    friend class sharp::Future;

    /**
     * Make friends with the ComposableFuture class because that uses private
     * members of this class
     */
    template <typename T>
    friend class sharp::detail::ComposableFuture;

private:

    void check_shared_state() const;
    template <typename Func>
    auto then_impl(Func&& func) -> Future<decltype(func(std::move(*this)))>;

    std::shared_ptr<detail::FutureImpl<Type>> shared_state;
};

} // namespace sharp

#include <sharp/Future/SharedFuture.ipp>
