/**
 * @file test_utf8_properties.c
 * @brief Property-based tests for UTF-8 utility functions.
 *
 * Verifies encode/decode roundtrips, substring inverses, and
 * delete/insert inverses across a range of Unicode codepoints.
 */
#include "../unity.h"
#include "core/utf8.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

void setUp(void) {}
void tearDown(void) {}

/* ------------------------------------------------------------------ */
/*  Property 1: UTF-8 encode then decode = original codepoint           */
/* ------------------------------------------------------------------ */

void test_property_utf8_encode_decode_roundtrip(void) {
    /* Test representative codepoints across the Unicode range */
    uint32_t test_cps[] = {
        0x0000, /* NULL */
        0x007F, /* ASCII max */
        0x0080, /* 2-byte start */
        0x07FF, /* 2-byte max */
        0x0800, /* 3-byte start */
        0xFFFF, /* BMP max */
        0x10000, /* Supplementary start */
        0x10FFFF, /* Unicode max */
        /* Common characters */
        'A', 'Z', 'a', 'z', '0', '9',
        0x00E9, /* e-acute */
        0x4E16, /* CJK: 世 */
        0x754C, /* CJK: 界 */
        0x1F389, /* Emoji: 🎉 */
    };

    char buf[8];
    for (size_t i = 0; i < sizeof(test_cps) / sizeof(test_cps[0]); i++) {
        uint32_t cp = test_cps[i];
        int bytes = utf8_encode(cp, buf);
        TEST_ASSERT_TRUE(bytes >= 1 && bytes <= 4);

        const char* p = buf;
        uint32_t decoded = utf8_decode(&p);
        TEST_ASSERT_EQUAL_INT(cp, decoded);
    }
}

/* ------------------------------------------------------------------ */
/*  Property 2: substring(0, len) = original string                     */
/* ------------------------------------------------------------------ */

void test_property_utf8_substring_inverse(void) {
    const char* test_strings[] = {
        "hello",
        "Hello World",
        "Hello \xE4\xB8\x96\xE7\x95\x8C",  /* Hello 世界 */
        "\xF0\x9F\x8E\x89",  /* Emoji */
        "",  /* empty string */
    };

    for (size_t i = 0; i < sizeof(test_strings) / sizeof(test_strings[0]); i++) {
        const char* str = test_strings[i];
        int byte_len = (int)strlen(str);
        int cp_count = utf8_codepoint_count(str, byte_len);

        /* Full substring should equal original */
        char* sub = utf8_substring(str, 0, cp_count);
        TEST_ASSERT_NOT_NULL(sub);
        TEST_ASSERT_EQUAL_STRING(str, sub);
        free(sub);

        /* Partial substring */
        if (cp_count > 1) {
            char* partial = utf8_substring(str, 0, cp_count - 1);
            TEST_ASSERT_NOT_NULL(partial);
            int partial_bytes = (int)strlen(partial);
            TEST_ASSERT_TRUE(partial_bytes < byte_len || byte_len == 0);
            free(partial);
        }
    }
}

/* ------------------------------------------------------------------ */
/*  Property 3: Delete range then insert same chars = original          */
/* ------------------------------------------------------------------ */

void test_property_utf8_delete_insert_inverse(void) {
    char buffer[1024];
    const char* original = "Hello, World!";
    int byte_len = (int)strlen(original);
    memcpy(buffer, original, (size_t)byte_len + 1);

    /* Save original for comparison */
    char original_copy[1024];
    memcpy(original_copy, original, (size_t)byte_len + 1);

    /* Delete "World!" (codepoints 7-12 inclusive in "Hello, World!") */
    int cp_count = utf8_codepoint_count(buffer, byte_len);
    TEST_ASSERT_EQUAL_INT(13, cp_count);

    /* Delete codepoints 7 through 13 (exclusive end: "World!") */
    int new_len = utf8_delete_range(buffer, byte_len, 7, 13);
    buffer[new_len] = '\0';
    TEST_ASSERT_EQUAL_STRING("Hello, ", buffer);

    /* Insert "World!" back */
    const char* to_insert = "World!";
    int final_len = utf8_insert(buffer, new_len, sizeof(buffer), 7, to_insert);
    TEST_ASSERT_TRUE(final_len > 0);
    buffer[final_len] = '\0';

    /* Should be back to original */
    TEST_ASSERT_EQUAL_STRING(original_copy, buffer);
}

/* ------------------------------------------------------------------ */
/*  Additional: codepoint count consistency                             */
/* ------------------------------------------------------------------ */

void test_property_utf8_codepoint_count_ascii(void) {
    const char* str = "abcde";
    int len = (int)strlen(str);
    TEST_ASSERT_EQUAL_INT(5, utf8_codepoint_count(str, len));
}

void test_property_utf8_byte_offset_roundtrip(void) {
    const char* str = "Hello";
    int byte_len = (int)strlen(str);
    int cp_count = utf8_codepoint_count(str, byte_len);

    /* For ASCII, cp index == byte offset */
    for (int i = 0; i < cp_count; i++) {
        int byte_off = utf8_cp_to_byte_offset(str, i);
        TEST_ASSERT_EQUAL_INT(i, byte_off);

        int cp_back = utf8_byte_to_cp_offset(str, byte_off);
        TEST_ASSERT_EQUAL_INT(i, cp_back);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_property_utf8_encode_decode_roundtrip);
    RUN_TEST(test_property_utf8_substring_inverse);
    RUN_TEST(test_property_utf8_delete_insert_inverse);
    RUN_TEST(test_property_utf8_codepoint_count_ascii);
    RUN_TEST(test_property_utf8_byte_offset_roundtrip);

    return UNITY_END();
}
