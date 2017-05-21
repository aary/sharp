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
              typename detail::EnableIfDoesNotReturnFuture<Func, Type>*
                  = nullptr>
    auto then(Func&& func) -> Future<decltype(func(std::move(*this)))>;
    template <typename Func,
              typename detail::EnableIfReturnsFuture<Func, Type>* = nullptr>
    auto then(Func&& func) -> decltype(func(std::move(*this)));

private:

    void check_shared_state() const;
    std::shared_ptr<detail::FutureImpl<Type>> shared_state;
    template <typename Func>
    auto then_impl(Func&& func) -> Future<decltype(func(std::move(*this)))>;
};

} // namespace sharp

#include <sharp/Future/SharedFuture.ipp>
