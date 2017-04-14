/**
 * @file FutureImpl.hpp
 * @author Aaryaman Sagar
 *
 * This file contains the main code for the FutureImpl class.  Which is the
 * implementation of the futures code, this contains all the synchronization
 * for when a value is set and stuff like that
 *
 * The Promise and Future classes are simply wrapper around this.  Both hook
 * into methods in this when one wants to set state and when the other waits
 * for the promise to be fulfilled.
 *
 * The Promise class is responsible for creating this shared state but the
 * Future class which in most cases extends beyond the Promise class is
 * responsible for destroying it
 */

#pragma once
