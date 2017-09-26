/**
 * @file Try.hpp
 * @author Aaryaman Sagar
 */

#pragma once

#include <sharp/Try/Try-pre.hpp>
#include <sharp/Utility/Utility.hpp>
#include <sharp/Portability/optional.hpp>

#include <stdexcept>
#include <type_traits>
#include <initializer_list>

namespace sharp {

/**
 * This error is thrown when the user tries to fetch a value or exception from
 * the Try when the Try does not have a value or exception to return
 */
class BadTryAccess : public std::logic_error {
public:
    using std::logic_error::logic_error;
};

/**
 * @class Try
 *
 * A Try represents either a value, an exception or an empty state.
 */
template <typename T, typename ExceptionPtr = std::exception_ptr>
class Try : public sharp::VariantMonad<T, ExceptionPtr> {
public:

    /**
     * Customary typedefs, similar to what std::optional provides
     */
    using value_type = T;

    /**
     * Default constructs a try, this has no significance, such a try has no
     * value and is essentially considered as an empty optional
     *
     * A try can be used as an optional as well since a default constructed
     * Try has no value or an exception
     */
    Try();

    /**
     * Destructor destroys the Try object by triggering the destructor for the
     * value contained if the Try contains a value, otherwise if the Try
     * contains an exception ~ExceptionPtr() is called.
     *
     * If the Try is in an empty state then the destructor is a no-op
     */
    ~Try();

    /**
     * Copy constructor constructs the current try from another Try object
     *
     * If the other Try has a value, then this Try will have a value as well,
     * copy constructed from the other value
     *
     * If the other Try has an exception then the current Try will contain a
     * copy of that excpetion_ptr
     *
     * If the other Try object does not have a value, this Try will also be
     * default constructed to have nothing and will behave as an empty
     * optional
     */
    Try(const Try&);

    /**
     * Move constructor emulates the copy constructor except that instead of
     * copying the value or an exception, it moves the them from the other Try
     * instance
     */
    Try(Try&&);

    /**
     * Constructs a Try from another Try object.  This retains the state of
     * the other Try.  So if the other Try had a value, this will also have
     * that value and the value will either be moved from the other try or it
     * will be copied based on the ref qualifiers of the other Try instance.
     *
     * If the other Try object holds an exception.  This will be constructed
     * with an exception as well by copy constructing the exception_ptr and
     * then destroying the exception_ptr held in the other Try
     *
     * In the case where the other Try did not have a value, this Try will
     * also not have a value and will be in an empty state
     *
     * Since this is a forwarding reference, the too perfect forwarding
     * problem has to be avoided, as usual
     */
    template <typename OtherTry,
              try_detail::EnableIfIsTry<OtherTry>* = nullptr,
              try_detail::EnableIfNotSelf<Try, OtherTry>* = nullptr>
    explicit Try(OtherTry&&);

    /**
     * Construct the Try from an object of type Type by move or copy
     * constructing the object internally.  This puts the Try in a state where
     * it holds a value and will return true for valid() and boolean conversions
     *
     *      auto something = Something{};
     *      auto try = Try<Something>{something};
     *      // try now has a copy of something
     */
    explicit Try(T&& instance);
    explicit Try(const T& instance);

    /**
     * Emplace construct a try with the arguments needed for the type
     *
     * For example
     *
     *      struct Something {
     *          explicit Something(int, double) { ... }
     *
     *          int a;
     *          double b;
     *      };
     *
     *      auto try = sharp::Try<Something>{std::in_place, 1, 1.0};
     *      cout << try.get().a << endl;
     *
     * This will call Something's explicit (int, double) constructor with the
     * given arguments forwared (albeit the forwarding makes no difference)
     */
    template <typename... Args>
    explicit Try(std::in_place_t, Args&&... args);
    template <typename U, typename... Args>
    explicit Try(std::in_place_t, std::initializer_list<U> il, Args&&... args);

    /**
     * Construct a Try with the given exception_ptr
     *
     * This will put the Try in a state where it contains an exception and
     * will return true for valid()
     *
     * Upon calling get() the Try will throw an exception
     */
    Try(ExceptionPtr ptr);

    /**
     * Construct a Try with a nullptr, this leaves the Try in a null state,
     * similar to std::optional<int>{std::nullopt};
     *
     * This will cause the Try to return false on valid() and will return
     * false for all boolean conversions
     */
    Try(std::nullptr_t);

    /**
     * Emplace the value into the Try with the arguments passed
     *
     * Returns a reference to the emplaced value
     */
    template <typename... Args>
    T& emplace(Args&&... args);
    template <typename U, typename... Args>
    T& emplace(std::initializer_list<U> il, Args&&... args);

    /**
     * Tests and checks whether the Try contains a value or an exception, if
     * the Try has either of those two then this will return true, else this
     * will return false
     *
     * is_ready() calls valid(), it is here to help make this type opaque with
     * Future::then() callbacks.  The user does not need to know whether a
     * future was passed to the callback or a Try
     */
    bool valid() const noexcept;
    bool is_ready() const noexcept;

    /**
     * Triggers a conversion to bool, similar to std::optional this will test
     * whether the Try contains either an exception or a value
     *
     *      auto try = sharp::Try<int>{nullptr};
     *      if (!try) {
     *          cout << "Tired of trying!" << endl;
     *      }
     *
     * In the example above, the print statement will be printed as the Try
     * does not contain anything
     */
    explicit operator bool() const noexcept;

    /**
     * Tests whether the Try has a value or an exception
     */
    bool has_value() const noexcept;
    bool has_exception() const noexcept;

    /**
     * Tries to get the value from the Try if the value exists, if the value
     * does not exist and the Try has an exception the exception is thrown
     * instead.
     *
     * If the Try does not have a value then a sharp::BadTryAccess error is
     * thrown to show a logic error on the programmers part
     */
    T& value() &;
    const T& value() const &;
    T&& value() &&;
    const T&& value() const &&;

    /**
     * get() does the same thing as value(), it is here to allow making this
     * type opaque and work with futures in .then() callbacks
     */
    T& get() &;
    const T& get() const &;
    T&& get() &&;
    const T&& get() const &&;

    /**
     * Pointer like functions to make the Try behave like a pointer and
     * returns the pointer to the contained value.
     *
     * Note that this function assumes that the Try contains a value, and will
     * result in undefined behavior if the Try either contains an excpetion or
     * does not contain anything
     */
    T& operator*() &;
    const T& operator*() const &;
    T&& operator*() &&;
    const T&& operator*() const &&;
    T* operator->();
    const T* operator->() const;

    /**
     * If the try contains an exception this function will return the
     * exception to the user.  This can be useful in cases where throwing the
     * exception over and over again might be expensive and throws should be
     * minimized
     *
     * If the Try does not contain an exception then a BadTryAccess error will
     * be thrown to reflect an error on the programmers part
     */
    ExceptionPtr exception() const;

private:

    using Super = VariantMonad<T, ExceptionPtr>;

    /**
     * Helper function that constructs the try from another try object
     */
    template <typename TryType>
    void construct_from_try(TryType&& other);

    /**
     * Throws the current exception being held internally in the exception_ptr
     * if there is one, otherwise a no-op
     */
    void throw_if_has_exception();

    /**
     * Implementation for the value() function
     */
    template <typename This>
    static decltype(auto) value_impl(This&& this_ref);

    /**
     * Implementation for the VariantMonad::get<>() function, this is shadowed
     * by the class's get() function so this is provided as a workaround
     */
    template <typename Type, typename This>
    static decltype(auto) get_monad(This&& this_ref);

    /**
     * The possible states for the Try
     */
    enum class State : std::int8_t {
        EMPTY,
        VALUE,
        EXCEPTION
    };

    /**
     * Storage used for the Try
     */
    State state{State::EMPTY};
};

} // namespace sharp

#include <sharp/Try/Try.ipp>
