#pragma once

#include <iostream>
#include <cassert>
#include <string>
#include <cstring>

// 测试计数器
extern int tests_run;
extern int tests_passed;

// 测试宏
#define ASSERT_TRUE(expr) \
    do { \
        tests_run++; \
        if (expr) { \
            tests_passed++; \
        } else { \
            std::cerr << "FAIL: " << #expr << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        } \
    } while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(a, b) \
    do { \
        tests_run++; \
        if ((a) == (b)) { \
            tests_passed++; \
        } else { \
            std::cerr << "FAIL: " << #a << " == " << #b << " at " << __FILE__ << ":" << __LINE__; \
            std::cerr << " (expected " << (b) << ", got " << (a) << ")" << std::endl; \
        } \
    } while(0)

#define ASSERT_NE(a, b) \
    do { \
        tests_run++; \
        if ((a) != (b)) { \
            tests_passed++; \
        } else { \
            std::cerr << "FAIL: " << #a << " != " << #b << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        } \
    } while(0)

#define ASSERT_GE(a, b) \
    do { \
        tests_run++; \
        if ((a) >= (b)) { \
            tests_passed++; \
        } else { \
            std::cerr << "FAIL: " << #a << " >= " << #b << " at " << __FILE__ << ":" << __LINE__; \
            std::cerr << " (expected >=" << (b) << ", got " << (a) << ")" << std::endl; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        tests_run++; \
        if ((ptr) != nullptr) { \
            tests_passed++; \
        } else { \
            std::cerr << "FAIL: " << #ptr << " != nullptr at " << __FILE__ << ":" << __LINE__ << std::endl; \
        } \
    } while(0)

#define ASSERT_STREQ(a, b) \
    do { \
        tests_run++; \
        if (std::strcmp((a), (b)) == 0) { \
            tests_passed++; \
        } else { \
            std::cerr << "FAIL: \"" << (a) << "\" == \"" << (b) << "\" at " << __FILE__ << ":" << __LINE__ << std::endl; \
        } \
    } while(0)

#define TEST(name) void name()

#define RUN_TESTS \
    do { \
        std::cout << "\n========================================" << std::endl; \
        std::cout << "Tests run: " << tests_run << ", Passed: " << tests_passed; \
        if (tests_run == tests_passed) { \
            std::cout << " [ALL PASSED]" << std::endl; \
        } else { \
            std::cout << " [" << (tests_run - tests_passed) << " FAILED]" << std::endl; \
        } \
        std::cout << "========================================" << std::endl; \
    } while(0)
