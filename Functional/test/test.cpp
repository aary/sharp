#include <memory>

#include <gtest/gtest.h>
#include <sharp/Functional/Functional.hpp>

TEST(Functional, BasicFunctional) {
    // move only lambda that would otherwise not be compatible with
    // std::function
    auto int_uptr = std::make_unique<int>(2);
    auto f = sharp::Function<int()>{[int_uptr = std::move(int_uptr)]() {
        return (*int_uptr) * 2;
    }};
    EXPECT_EQ(f(), 4);
}

// #include <iostream>
// #include <cstdlib>
// #include <memory>
// #include <cstdlib>

// using namespace std;

// struct profiler
// {
    // std::string name;
    // std::chrono::high_resolution_clock::time_point p;
    // profiler(std::string const &n) :
        // name(n), p(std::chrono::high_resolution_clock::now()) { }
    // ~profiler()
    // {
        // using dura = std::chrono::duration<double>;
        // auto d = std::chrono::high_resolution_clock::now() - p;
        // std::cout << name << ": "
            // << std::chrono::duration_cast<dura>(d).count()
            // << std::endl;
    // }
// };
// #define PROFILE_BLOCK(pbn) profiler _pfinstance(pbn)

// TEST(Functional, PerformanceTest) {

    // std::srand(std::time(0));
    // int x{0};
    // for(int xx = 0; xx < 5; ++xx)
    // {
        // {
            // PROFILE_BLOCK("std::function");
            // for (auto i = 0; i < 1000; ++i) {
                // x = std::rand();
                // std::function<int(int)> t2 = [&x](int i){ return i + x; };
                // std::function<void(int)> t1 = [&x, &t2](int i){ x = t2(i); };
                // for(int j = 0; j < 1000000; ++j) t1(j);
            // }
        // }

        // {
            // PROFILE_BLOCK("sharp::Function");
            // for (auto i = 0; i < 1000; ++i) {
                // x = std::rand();
                // sharp::Function<int(int)> t2 = [&x](int i){ return i + x; };
                // sharp::Function<void(int)> t1 = [&x, &t2](int i){ x = t2(i); };
                // for(int j = 0; j < 1000000; ++j) t1(j);
            // }
        // }
    // }
// }

