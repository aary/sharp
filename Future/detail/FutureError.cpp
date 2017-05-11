#include <system_error>
#include <string>

#include <sharp/Future/Future.hpp>

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
class FutureErrorCategory : std::error_category {
public:
    const char* name() noexcept const override;
    std::string message(int value) const override;
};

const char* FutureErrorCategory::name() noexcept const override {
    return "Future::FutureErrorCategory";
}

std::string FutureErrorCategory::message(int value) const override {
    switch (FutureErrorCode{value}) {
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
    const static FutureErrorCategory FUTURE_ERROR_CATEGORY;
    return FUTURE_ERROR_CATEGORY;
}

} // namespace sharp
