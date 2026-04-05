#ifndef UNITY_H
#define UNITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int tests_run;
extern int tests_failed;

#define UNITY_BEGIN() (tests_run = 0, tests_failed = 0, printf("=== Running Tests ===\n"))
#define UNITY_END() (printf("\n=== Results: %d run, %d failed ===\n", tests_run, tests_failed), tests_failed)

#define RUN_TEST(func) do { \
    tests_run++; \
    printf("  %-50s ", #func); \
    setUp(); \
    func(); \
    tearDown(); \
    printf("PASS\n"); \
} while(0)

#define TEST_ASSERT_NULL(val) do { \
    if ((val) != NULL) { \
        printf("FAIL - expected NULL, got %p\n", (void*)(val)); \
        tests_failed++; return; \
    } \
} while(0)

#define TEST_ASSERT_NOT_NULL(val) do { \
    if ((val) == NULL) { \
        printf("FAIL - expected non-NULL\n"); \
        tests_failed++; return; \
    } \
} while(0)

#define TEST_ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        printf("FAIL - assertion " #cond " failed\n"); \
        tests_failed++; return; \
    } \
} while(0)

#define TEST_ASSERT_FALSE(cond) do { \
    if (cond) { \
        printf("FAIL - expected false condition\n"); \
        tests_failed++; return; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_INT(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("FAIL - expected %d, got %d\n", (int)(expected), (int)(actual)); \
        tests_failed++; return; \
    } \
} while(0)

#define TEST_ASSERT_GREATER_THAN(threshold, actual) do { \
    if ((actual) <= (threshold)) { \
        printf("FAIL - expected > %d, got %d\n", (int)(threshold), (int)(actual)); \
        tests_failed++; return; \
    } \
} while(0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("FAIL - expected '%s', got '%s'\n", (expected), (actual)); \
        tests_failed++; return; \
    } \
} while(0)

void setUp(void);
void tearDown(void);

#endif
