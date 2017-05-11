/**
 * @file FutureError.hpp
 * @author Aaryaman Sagar
 */

namespace sharp {

/**
 * @enum FutureErrorCode
 *
 * This enum contains conditions erroneous conditions that can be exhibited
 * when using the future and promise APIs
 *
 * broken_promise is the error that is thrown when a promise is destroyed
 *                without being sent either a value or an exception
 * future_already_retrieved when get() is called on a future that has already
 *                          been retrieved
 * promise_already_satisfied when you try and store a value or exception in a
 *                           promise that already has one of those stored
 * no_state attempt to access future or promise methods when there is no
 *          shared state
 */
enum class FutureErrorCode : int {
    broken_promise,
    future_already_retrieved,
    promise_already_satisfied,
    no_state
};

/**
 * @function future_error_category
 *
 * This function returns a singleton error_category instance that can be used
 * as the representative error_category object for Futures
 */
const std::error_category& future_error_category();

} // namespace sharp

namespace std {

    /**
     * Overload for is_error_code_enum for traits purposes
     */
    template <>
    struct is_error_code_enum<sharp::FutureErrorCode> : std::true_type {};

    /**
     * Overload make_error_code and make_error_condition as is needed for
     * every custom error class
     */
    inline error_code make_error_code(sharp::FutureErrorCode err) noexcept {
        return std::error_code{static_cast<int>(err),
            sharp::future_error_category()};
    }
    inline error_condition make_error_condition(sharp::FutureErrorCode err)
            noexcept {
        return error_condition{static_cast<int>(err),
            sharp::future_error_category()};
    }
} // namespace std

namespace sharp {

class FutureError : std::logic_error {
public:
    explicit FutureError(FutureErrorCode err)
        : std::logic_error{"sharp::FutureError: "
            + std::make_error_code(err).message()},
        code_object{std::make_error_code(err)} {}
    const std::error_code& code() const {
        return this->code_object;
    }
    const char* what() const noexcept override {
        return "Future error";
    }

private:
    std::error_code code_object;
};

} // namespace sharp
