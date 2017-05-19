/**
 * @file Function.hpp
 * @author Aaryaman Sagar
 *
 * This file contains an implementation of std::function that is based off a
 * stackoverflow (codeexchange to be specific) post, which is itself an
 * implementation of the concept described in the article "The impossibly fast
 * C++ delegates) here
 * https://www.codeproject.com/Articles/11015/The-Impossibly-Fast-C-Delegates
 *
 * And the code for this is hosted on stackexchange here
 * https://codereview.stackexchange.com/questions/14730
 *
 * There are a couple of reasons std::function might not work
 *  1. It cannot be used with move only function objects.  Move only function
 *     objects are very common in C++14 and beyond since lambdas with
 *     generalized move captures cannot be copied, for instance
 *
 *     auto u_ptr = std::make_unique<int>{1};
 *     auto f = std::function<void()>{[u_ptr = std::move(u_ptr)]() {}};
 *
 *     will not compile because the lambda in the constructor is move only
 *
 *  2. std::function is about 2x slower than this on my computer (Apple LLVM
 *     version 8.0.0 (clang-800.0.42.1)), wow I know...  the test code for this
 *     claim is in the comments way at the bottom
 */

#pragma once

#include <cassert>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace sharp {

template <typename T> class Function;

template<class R, class ...A>
class Function<R (A...)>
{
  using stub_ptr_type = R (*)(void*, A&&...);

  Function(void* const o, stub_ptr_type const m) noexcept :
    object_ptr_(o),
    stub_ptr_(m)
  {
  }

public:
  Function() = default;

  Function(Function const&) = default;

  Function(Function&&) = default;

  Function(::std::nullptr_t const) noexcept : Function() { }

  template <class C, typename =
    typename ::std::enable_if< ::std::is_class<C>{}>::type>
  explicit Function(C const* const o) noexcept :
    object_ptr_(const_cast<C*>(o))
  {
  }

  template <class C, typename =
    typename ::std::enable_if< ::std::is_class<C>{}>::type>
  explicit Function(C const& o) noexcept :
    object_ptr_(const_cast<C*>(&o))
  {
  }

  template <class C>
  Function(C* const object_ptr, R (C::* const method_ptr)(A...))
  {
    *this = from(object_ptr, method_ptr);
  }

  template <class C>
  Function(C* const object_ptr, R (C::* const method_ptr)(A...) const)
  {
    *this = from(object_ptr, method_ptr);
  }

  template <class C>
  Function(C& object, R (C::* const method_ptr)(A...))
  {
    *this = from(object, method_ptr);
  }

  template <class C>
  Function(C const& object, R (C::* const method_ptr)(A...) const)
  {
    *this = from(object, method_ptr);
  }

  template <
    typename T,
    typename = typename ::std::enable_if<
      !::std::is_same<Function, typename ::std::decay<T>::type>{}
    >::type
  >
  Function(T&& f) :
    store_(operator new(sizeof(typename ::std::decay<T>::type)),
      functor_deleter<typename ::std::decay<T>::type>),
    store_size_(sizeof(typename ::std::decay<T>::type))
  {
    using functor_type = typename ::std::decay<T>::type;

    new (store_.get()) functor_type(::std::forward<T>(f));

    object_ptr_ = store_.get();

    stub_ptr_ = functor_stub<functor_type>;

    deleter_ = deleter_stub<functor_type>;
  }

  Function& operator=(Function const&) = default;

  Function& operator=(Function&&) = default;

  template <class C>
  Function& operator=(R (C::* const rhs)(A...))
  {
    return *this = from(static_cast<C*>(object_ptr_), rhs);
  }

  template <class C>
  Function& operator=(R (C::* const rhs)(A...) const)
  {
    return *this = from(static_cast<C const*>(object_ptr_), rhs);
  }

  template <
    typename T,
    typename = typename ::std::enable_if<
      !::std::is_same<Function, typename ::std::decay<T>::type>{}
    >::type
  >
  Function& operator=(T&& f)
  {
    using functor_type = typename ::std::decay<T>::type;

    if ((sizeof(functor_type) > store_size_) || !store_.unique())
    {
      store_.reset(operator new(sizeof(functor_type)),
        functor_deleter<functor_type>);

      store_size_ = sizeof(functor_type);
    }
    else
    {
      deleter_(store_.get());
    }

    new (store_.get()) functor_type(::std::forward<T>(f));

    object_ptr_ = store_.get();

    stub_ptr_ = functor_stub<functor_type>;

    deleter_ = deleter_stub<functor_type>;

    return *this;
  }

  template <R (* const function_ptr)(A...)>
  static Function from() noexcept
  {
    return { nullptr, function_stub<function_ptr> };
  }

  template <class C, R (C::* const method_ptr)(A...)>
  static Function from(C* const object_ptr) noexcept
  {
    return { object_ptr, method_stub<C, method_ptr> };
  }

  template <class C, R (C::* const method_ptr)(A...) const>
  static Function from(C const* const object_ptr) noexcept
  {
    return { const_cast<C*>(object_ptr), const_method_stub<C, method_ptr> };
  }

  template <class C, R (C::* const method_ptr)(A...)>
  static Function from(C& object) noexcept
  {
    return { &object, method_stub<C, method_ptr> };
  }

  template <class C, R (C::* const method_ptr)(A...) const>
  static Function from(C const& object) noexcept
  {
    return { const_cast<C*>(&object), const_method_stub<C, method_ptr> };
  }

  template <typename T>
  static Function from(T&& f)
  {
    return ::std::forward<T>(f);
  }

  static Function from(R (* const function_ptr)(A...))
  {
    return function_ptr;
  }

  template <class C>
  using member_pair =
    ::std::pair<C* const, R (C::* const)(A...)>;

  template <class C>
  using const_member_pair =
    ::std::pair<C const* const, R (C::* const)(A...) const>;

  template <class C>
  static Function from(C* const object_ptr,
    R (C::* const method_ptr)(A...))
  {
    return member_pair<C>(object_ptr, method_ptr);
  }

  template <class C>
  static Function from(C const* const object_ptr,
    R (C::* const method_ptr)(A...) const)
  {
    return const_member_pair<C>(object_ptr, method_ptr);
  }

  template <class C>
  static Function from(C& object, R (C::* const method_ptr)(A...))
  {
    return member_pair<C>(&object, method_ptr);
  }

  template <class C>
  static Function from(C const& object,
    R (C::* const method_ptr)(A...) const)
  {
    return const_member_pair<C>(&object, method_ptr);
  }

  void reset() { stub_ptr_ = nullptr; store_.reset(); }

  void reset_stub() noexcept { stub_ptr_ = nullptr; }

  void swap(Function& other) noexcept { ::std::swap(*this, other); }

  bool operator==(Function const& rhs) const noexcept
  {
    return (object_ptr_ == rhs.object_ptr_) && (stub_ptr_ == rhs.stub_ptr_);
  }

  bool operator!=(Function const& rhs) const noexcept
  {
    return !operator==(rhs);
  }

  bool operator<(Function const& rhs) const noexcept
  {
    return (object_ptr_ < rhs.object_ptr_) ||
      ((object_ptr_ == rhs.object_ptr_) && (stub_ptr_ < rhs.stub_ptr_));
  }

  bool operator==(::std::nullptr_t const) const noexcept
  {
    return !stub_ptr_;
  }

  bool operator!=(::std::nullptr_t const) const noexcept
  {
    return stub_ptr_;
  }

  explicit operator bool() const noexcept { return stub_ptr_; }

  R operator()(A... args) const
  {
//  assert(stub_ptr);
    return stub_ptr_(object_ptr_, ::std::forward<A>(args)...);
  }

private:
  friend struct ::std::hash<Function>;

  using deleter_type = void (*)(void*);

  void* object_ptr_;
  stub_ptr_type stub_ptr_{};

  deleter_type deleter_;

  ::std::shared_ptr<void> store_;
  ::std::size_t store_size_;

  template <class T>
  static void functor_deleter(void* const p)
  {
    static_cast<T*>(p)->~T();

    operator delete(p);
  }

  template <class T>
  static void deleter_stub(void* const p)
  {
    static_cast<T*>(p)->~T();
  }

  template <R (*function_ptr)(A...)>
  static R function_stub(void* const, A&&... args)
  {
    return function_ptr(::std::forward<A>(args)...);
  }

  template <class C, R (C::*method_ptr)(A...)>
  static R method_stub(void* const object_ptr, A&&... args)
  {
    return (static_cast<C*>(object_ptr)->*method_ptr)(
      ::std::forward<A>(args)...);
  }

  template <class C, R (C::*method_ptr)(A...) const>
  static R const_method_stub(void* const object_ptr, A&&... args)
  {
    return (static_cast<C const*>(object_ptr)->*method_ptr)(
      ::std::forward<A>(args)...);
  }

  template <typename>
  struct is_member_pair : std::false_type { };

  template <class C>
  struct is_member_pair< ::std::pair<C* const,
    R (C::* const)(A...)> > : std::true_type
  {
  };

  template <typename>
  struct is_const_member_pair : std::false_type { };

  template <class C>
  struct is_const_member_pair< ::std::pair<C const* const,
    R (C::* const)(A...) const> > : std::true_type
  {
  };

  template <typename T>
  static typename ::std::enable_if<
    !(is_member_pair<T>{} ||
    is_const_member_pair<T>{}),
    R
  >::type
  functor_stub(void* const object_ptr, A&&... args)
  {
    return (*static_cast<T*>(object_ptr))(::std::forward<A>(args)...);
  }

  template <typename T>
  static typename ::std::enable_if<
    is_member_pair<T>{} ||
    is_const_member_pair<T>{},
    R
  >::type
  functor_stub(void* const object_ptr, A&&... args)
  {
    return (static_cast<T*>(object_ptr)->first->*
      static_cast<T*>(object_ptr)->second)(::std::forward<A>(args)...);
  }
};

} // namespace sharp

namespace std
{
  template <typename R, typename ...A>
  struct hash<sharp::Function<R (A...)> >
  {
    size_t operator()(sharp::Function<R (A...)> const& d) const noexcept
    {
      auto const seed(hash<void*>()(d.object_ptr_));

      return hash<typename sharp::Function<R (A...)>::stub_ptr_type>()(
        d.stub_ptr_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
  };
} // namespace std

/**
#include <iostream>
#include <cstdlib>
#include <memory>

#include "function.hpp"

using namespace std;

struct profiler
{
    std::string name;
    std::chrono::high_resolution_clock::time_point p;
    profiler(std::string const &n) :
        name(n), p(std::chrono::high_resolution_clock::now()) { }
    ~profiler()
    {
        using dura = std::chrono::duration<double>;
        auto d = std::chrono::high_resolution_clock::now() - p;
        std::cout << name << ": "
            << std::chrono::duration_cast<dura>(d).count()
            << std::endl;
    }
};
#define PROFILE_BLOCK(pbn) profiler _pfinstance(pbn)

int main() {

    std::srand(std::time(0));
    int x{0};
    for(int xx = 0; xx < 5; ++xx)
    {
        {
            PROFILE_BLOCK("std::function");
            for (auto i = 0; i < 1000; ++i) {
                x = std::rand();
                std::function<int(int)> t2 = [&x](int i){ return i + x; };
                std::function<void(int)> t1 = [&x, &t2](int i){ x = t2(i); };
                for(int j = 0; j < 1000000; ++j) t1(j);
            }
        }

        {
            PROFILE_BLOCK("delegate");
            for (auto i = 0; i < 1000; ++i) {
                x = std::rand();
                delegate<int(int)> t2 = [&x](int i){ return i + x; };
                delegate<void(int)> t1 = [&x, &t2](int i){ x = t2(i); };
                for(int j = 0; j < 1000000; ++j) t1(j);
            }
        }
    }

    return 0;
}
*/
