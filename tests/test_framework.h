#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace testing {

struct Failure {
    std::string message;
};

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> cases;
    return cases;
}

struct Registrar {
    Registrar(std::string name, std::function<void()> fn) {
        registry().push_back(TestCase{std::move(name), std::move(fn)});
    }
};

inline int run_all() {
    int failures = 0;
    for (const TestCase& test : registry()) {
        try {
            test.fn();
            std::cout << "[PASS] " << test.name << '\n';
        } catch (const Failure& failure) {
            ++failures;
            std::cout << "[FAIL] " << test.name << ": " << failure.message << '\n';
        } catch (const std::exception& e) {
            ++failures;
            std::cout << "[FAIL] " << test.name << ": unexpected exception: " << e.what()
                      << '\n';
        }
    }
    std::cout << (failures == 0 ? "ALL TESTS PASSED" : "TESTS FAILED") << " ("
              << registry().size() << " cases)\n";
    return failures == 0 ? 0 : 1;
}

} // namespace testing

#define HAN_TEST(name)                                             \
    static void name();                                            \
    static ::testing::Registrar name##_registrar(#name, name);     \
    static void name()

#define HAN_CHECK(condition)                                                          \
    do {                                                                              \
        if (!(condition)) {                                                           \
            throw ::testing::Failure{std::string(__FILE__) + ":" +                    \
                                     std::to_string(__LINE__) +                       \
                                     " check failed: " #condition};                   \
        }                                                                             \
    } while (0)

#define HAN_CHECK_EQ(lhs, rhs)                                                        \
    do {                                                                              \
        const auto& han_lhs = (lhs);                                                  \
        const auto& han_rhs = (rhs);                                                  \
        if (!(han_lhs == han_rhs)) {                                                  \
            std::ostringstream han_stream;                                            \
            han_stream << __FILE__ << ":" << __LINE__ << " expected " #lhs " == " #rhs \
                       << " but got [" << han_lhs << "] vs [" << han_rhs << "]";      \
            throw ::testing::Failure{han_stream.str()};                                \
        }                                                                             \
    } while (0)
