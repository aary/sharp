#include <system_error>
#include <string>
#include <cassert>

#include "FutureError.hpp"

namespace sharp {

namespace detail {
    const std::string BROKEN_PROMISE{"broken promise"};
    const std::string FUTURE_ALREADY_RETRIEVED{"future already retrieved"};
    const std::string PROMISE_ALREADY_SATISFIED{"promise already satisfied"};
    const std::string NO_STATE{"no state"};
} // namespace detail

/**
 * @class FutureErrorCategory
 *
 * The class that is the representative error category for future related
 * errors
 */
class FutureErrorCategory : public std::error_category {
public:
    FutureErrorCategory() : std::error_category{} {}
    const char* name() const noexcept override;
    std::string message(int value) const override;
};

const char* FutureErrorCategory::name() const noexcept {
    return "Future::FutureErrorCategory";
}

std::string FutureErrorCategory::message(int value) const {

    // assert that the integer passed to FutureErrorCategory is within the
    // range of the enumeration, otherwise there will be undefined behavior
    assert(value <= static_cast<int>(FutureErrorCode::no_state));
    switch (static_cast<FutureErrorCode>(value)) {
        case FutureErrorCode::broken_promise:
            return detail::BROKEN_PROMISE;
            break;
        case FutureErrorCode::future_already_retrieved:
            return detail::FUTURE_ALREADY_RETRIEVED;
            break;
        case FutureErrorCode::promise_already_satisfied:
            return detail::PROMISE_ALREADY_SATISFIED;
            break;
        case FutureErrorCode::no_state:
            return detail::NO_STATE;
            break;
    }
}

const std::error_category& future_error_category() {
    static const FutureErrorCategory FUTURE_ERROR_CATEGORY;
    return FUTURE_ERROR_CATEGORY;
}

} // namespace sharp
