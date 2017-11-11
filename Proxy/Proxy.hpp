/**
 * @file Proxy.hpp
 * @author Aaryaman Sagar
 *
 * A generic proxy class that encapsulates state, exposes it as a
 * "smart pointer" and executes some logic on destruction
 *
 * This is a generic unqiue_ptr type of class that can be used even when the
 * objects are stored on the stack, unlike std::unique_ptr which requires
 * objecrs to be stored on the heap and used with an allocator
 *
 * This can also be used as a wrapper around pointer-like objects, in which
 * case the pointer operations will be forwarded to that pointer type
 */

#pragma once

/**
 * @class Proxy
 *
 * A generic RAII wrapper class that associates state on construction with a
 * data member.  This state is then exposed via a pointer like interface and
 * then released on destruction with an on exit callback
 *
 * The proxy class can wrap objects allocated inline as well as externally
 * through other smart pointers (possibly on the heap)
 *
 *      auto proxy = sharp::make_proxy([] { return 1; }, [](int i) { log(i); });
 *      cout << "Value in the proxy is " << *proxy;
 *
 *      auto proxy = sharp::make_proxy(
 *          [] { return std::make_unique<int>(1); },
 *          [](auto integer) { log(integer); }};
 *      cout << "Value in the proxy is " << *proxy << endl;
 *
 * This sort of pointer like operator->() and operator* might not be desirable
 * so the class allows you to fetch references to the underlying data item via
 * a get() method
 *
 *      auto proxy = make_int_proxy();
 *      static_assert(std::is_same<decltype(proxy.get()), int&>::value);
 *
 *      auto proxy = make_unique_ptr_int_proxy();
 *      static_assert(std::is_same<decltype(proxy.get()),
 *                                 std::unique_ptr<int>&>::value);
 *
 * This should be constructed with the make_proxy() method which accepts two
 * callables - one which returns the thing that is to be stored in the proxy a
 * callable that will be invoked when the proxy is destroyed
 *
 * To wrap references, the creater function object passed to make_proxy should
 * return an object wrapped in a reference_wrapper this class will take care
 * of unwrapping the reference
 *
 *      auto proxy = sharp::make_proxy([] { return std::ref(a); }, [](auto&){});
 *
 * Now the object being wrapped in the proxy will be a reference, and access
 * to it will be controlled through the same pointer like syntax
 */
template <typename T, typename OnExit>
class Proxy {};
