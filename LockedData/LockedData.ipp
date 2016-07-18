#include "LockedData.hpp"

#include <mutex>
#include <condition_variable>
#include <utility>
#include <type_traits>

namespace sharp {

/**
 * @class UniqueLockedProxy A proxy class for the internal representation of
 *                          the data object in LockedData that automates
 *                          locking and unlocking of the internal mutex in
 *                          write (non const) scenarios.
 *
 * A proxy class that is locked by the non-const locking policy of the given
 * mutex on construction and unlocked on destruction.
 *
 * So for example when the mutex is a shared lock or a reader writer lock then
 * the internal implementation will choose to write lock the object because
 * the LockedData is not const.
 *
 * TODO If and when the operator.() becomes a thing support should be added to
 * make this a proper proxy.
 */
template <typename Type, typename Mutex>
class LockedData::UniqueLockedProxy {};

/**
 * @class ConstUniqueLockedProxy A proxy class for LockedData that automates
 *                               locking and unlocking of the internal mutex.
 *
 * A proxy class that is locked by the const locking policy of the given mutex
 * on construction and unlocked on destruction.
 *
 * So for example when the mutex is a shared lock or a reader writer lock then
 * the internal implementation will choose to read lock the object because
 * the LockedData object is const and therefore no write access will be
 * granted.
 *
 * This should not be used by the implementation when the internal object is
 * not const.
 *
 * TODO If and when the operator.() becomes a thing support should be added to
 * make this a proper proxy.
 */
template <typename Type, typename Mutex>
class LockedData::ConstUniqueLockedProxy {};

} // namespace sharp
