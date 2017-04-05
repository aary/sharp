/**
 * @file Tags.hpp
 * @author Aaryaman Sagar (rmn100@gmail.com)
 *
 * This module contains simple tags that I have used to be explicit with tag
 * dispatch in some cases, for example tag dispatch can be used to fix the
 * ambiguity introdced by construction with std::initializer_list like so
 *
 *  Example example{initializer_list_construct, {1, 2, 3, 4, 5}};
 *
 * Here the Example class can be defined like follows
 *
 *  class Example {
 *  public:
 *      Example(initializer_list_construct::tag_t, std::initializer_list<int>)
 *  };
 *
 * So it is clear that the initializer list constructor is being called.  This
 * sort of thing can help with template code where it is not clear what
 * sequence of elements comprise of a valid constructor call for the container
 * object.
 */

#pragma once

namespace sharp {

/**
 * @class GeneralizedTag
 *
 * A generic tag type that can be used conveniently to create a new
 * generalized tag instance.  For example the user may want to pass in a tag
 * with a given type, for example
 *
 *  sharp::Variant<string, int, double> var {
 *      sharp::emplace_construct::tag<string>, "some string"};
 *
 * it can pass in an integral constant
 *
 *  sharp::Variant<string, int, double> var {
 *      sharp::emplace_construct::tag<2>, 1.0};
 *
 * or it can simply be an empty tag
 *
 *  sharp::LockedData<vector<int>> locked_vector {
 *      sharp::emplace_construct::tag, {1, 2, 3, 4}};
 *
 * The implementation uses function pointers as described here
 *  http://en.cppreference.com/w/cpp/utility/in_place
 *
 * Extend this class via CRTP (goo.gl/uGrvZC) like the following
 *
 *  class new_tag : public GeneralizedTag<new_tag> {};
 */
template <typename Derived>
class GeneralizedTag {
private:

    /**
     * classes used to define the input type to the functions to separate them
     * for each derived instance
     *
     * since this type is defined within the scope of the template
     * instantiation, it will be unique for each tag class instantiated with
     * this as the base
     */
    class InputTag {};
    template <typename Type>
    class InputTagType {};
    template <int Integer>
    class InputTagIntegral {};

public:

    /**
     * The tag types that are going to be used within the API (either
     * privately or publically) to provide tag dispatch functionality to the
     * user
     */
    using tag_t = void (*) (const InputTag&);
    template <typename Type>
    using tag_type_t = void (*) (const InputTagType<Type>&);
    template <int INTEGER>
    using tag_integral_t = void (*) (const InputTagIntegral<INTEGER>&);

    /**
     * The actual tags to be used by the user.
     */
    static void tag(const InputTag& = InputTag{}) {}
    template <typename Type>
    static void tag_type(const InputTagType<Type>& = InputTagType<Type>{}) {}
    template <int INTEGER>
    static void tag_integral(
            const InputTagIntegral<INTEGER>& = InputTagIntegral<INTEGER>{}) {}
};

/**
 * @class initializer_list_construct
 *
 * The original version of the tag to disambiguate inistializer list
 * construction that also made it to Facebook's open source C++ library folly.
 *
 * See here
 * https://github.com/facebook/folly/blob/master/folly/Traits.h#L576
 *
 * This tag deals with the ambiguity introduced by std::initializer_list and
 * the uniform initialization syntax.  Now that the syntax has been
 * standardized and has been included in three consecutive standard changes
 * (C++11, C++14 and C++17) it seems unlikely that this ambiguity is going to
 * be fixed.  So this is a simple construct that helps in disambiguating the
 * use of std::initializer_list in constructors.  This can also be helpful in
 * template code where it is not clear what sequence of elements comprise of a
 * valid constructor call for the container object.
 *
 * The following is a good example and makes a strong case for why this tag
 * should be enforced
 *
 *  class Something {
 *  public:
 *    explicit Something(int);
 *    Something(std::intiializer_list<int>);
 *
 *    operator int();
 *  };
 *
 *  ...
 *
 *  // SURPRISE!
 *  Something something{1};
 *  Soemthing something_else{something};
 *
 * The last call to instantiate the Something object will go to the
 * initializer_list overload.  Which may be surprising to users.
 *
 * If however this tag was used to disambiguate such construction it would be
 * easy for users to see which construction overload their code was referring
 * to.  For example
 *
 *  class Something {
 *  public:
 *    explicit Something(int);
 *    Something(folly::initlist_construct::tag_t, std::initializer_list<int>);
 *
 *    operator int();
 *  };
 *
 *  ...
 *  Something something_one{1}; // not the initializer_list overload
 *  Something something_two{folly::initlist_construct::tag, {1}}; // correct
 *
 * Note that const objects have internal linkage so they do not introduce
 * linker errors when multiple cpp files include this header file.
 */
class initializer_list_construct :
    public GeneralizedTag<initializer_list_construct> {};

/**
 * @class emplace_construct
 *
 * Tag used to disambiguate construction with variadic arguments.  This is
 * especially useful in simple wrappers which simulate functionality of
 * aggregates.
 *
 * For example
 *
 *  template <typename Type>
 *  class SimulatedAggregate { ... };
 *
 *  SimulatedAggregate<Complex> object{emplace_construct,
 *      std::forward<Args>(args)...};
 *
 * This can be used to show that the arguments are going to be forwarded
 * straight to the inner element stored in the other class, in this case
 * SimulatedAggregate
 */
class emplace_construct : public GeneralizedTag<emplace_construct> {};

/**
 * @class delegate_constructor
 *
 * A tag used for RAII based constructor decoration.  This technique can be
 * described as executing code before and after initialization of a class's
 * contained members.  Since the only methods and functions that are allowed
 * to be called before element initialization in a constructor are other
 * constructors this tag can be used to explicitly show that another
 * constructor is being used just to show that code is executing before
 * initialization
 */
class delegate_constructor : public GeneralizedTag<delegate_constructor> {};

/**
 * @class implementation
 *
 * A tag used to seperate the handle from the actual underlying
 * implementation.  This is more versatile than actually disambiguating
 * between handle and implementation by virtue of different but related names
 * primarily because it allows usage in more situations, for example template
 * specialization and constructor delegation.  For usage with constructors
 * this can be considered a part of the idiom described with
 * delegate_constructor_t
 */
class implementation : public GeneralizedTag<implementation> {};

/**
 * @class preferred_dispatch
 *
 * A module that can be used to implement compile time preferred dispatch in
 * function overloads.
 *
 * For example if you have 3 ways to search a container, the first most
 * preferred approach being a constant time lookup, the second being a
 * logarithmic time lookup and the third being a linear time lookup, one could
 * leverage SFINAE and preferred dispatch like so
 *
 *      template <typename Container, typename Value,
 *                EnableIfHasConstantTimeLookup<Container>* = nullptr>
 *      auto lookup_impl(Container& container, const Value& value,
 *          sharp::preferred_dispatch<2>::tag_t);
 *
 *      template <typename Container, typename Value,
 *                EnableIfHasLogTimeLookup<Container>* = nullptr>
 *      auto lookup_impl(Container& container, const Value& value,
 *          sharp::preferred_dispatch<1>::tag_t);
 *
 *      template <typename Container, typename Value,
 *                EnableIfHasLinearTimeLookup<Container>* = nullptr>
 *      auto lookup_impl(Container& container, const Value& value,
 *          sharp::preferred_dispatch<0>::tag_t);
 *
 * Then a user may call these implementation functions like this
 *      auto iter = lookup_impl(container, value,
 *          sharp::preferred_dispatch<2>::tag);
 *
 * to specify that they want the implementation with the highest priority to
 * be picked first, i.e. try the implementation with the 2, if that fails then
 * fall back to the implementation with the priority 1, etc.
 */
template <int priority>
class preferred_dispatch : public preferred_dispatch<priority - 1> {
    static_assert(priority >= 0, "Priority cannot be negative");
};
template <>
class preferred_dispatch<0> {};

/**
 * @class empty
 *
 * This tag is used to disambiguate constructors or factory functions that
 * create an empty instance of a type.  This empty type is then used by a non
 * constructor function to give state to the instance.  This is useful in
 * cases like factory functions, etc
 */
class empty : public GeneralizedTag<empty> {};

} // namespace sharp
