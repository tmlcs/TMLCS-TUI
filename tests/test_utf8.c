#include "unity.h"
#include "core/utf8.h"
#include <string.h>
#include <stdlib.h>

int tests_run = 0;
int tests_failed = 0;

void setUp(void) {}
void tearDown(void) {}

/* ------------------------------------------------------------------ */
/*  utf8_encode tests                                                    */
/* ------------------------------------------------------------------ */

void test_utf8_encode_ascii(void) {
    char out[4];
    memset(out, 0, sizeof(out));
    int len = utf8_encode('A', out);
    TEST_ASSERT_EQUAL_INT(1, len);
    TEST_ASSERT_EQUAL_INT('A', (unsigned char)out[0]);
}

void test_utf8_encode_2byte(void) {
    char out[4];
    int len = utf8_encode(0x00E9, out);  /* e-acute */
    TEST_ASSERT_EQUAL_INT(2, len);
    /* Verify roundtrip */
    const char* p = out;
    uint32_t cp = utf8_decode(&p);
    TEST_ASSERT_EQUAL_INT(0x00E9, cp);
}

void test_utf8_encode_3byte(void) {
    char out[4];
    int len = utf8_encode(0x4E2D, out);  /* 中 */
    TEST_ASSERT_EQUAL_INT(3, len);
    const char* p = out;
    uint32_t cp = utf8_decode(&p);
    TEST_ASSERT_EQUAL_INT(0x4E2D, cp);
}

void test_utf8_encode_4byte(void) {
    char out[4];
    int len = utf8_encode(0x1F389, out);  /* party popper */
    TEST_ASSERT_EQUAL_INT(4, len);
    const char* p = out;
    uint32_t cp = utf8_decode(&p);
    TEST_ASSERT_EQUAL_INT(0x1F389, cp);
}

void test_utf8_decode_roundtrip(void) {
    uint32_t test_cps[] = {0x00, 0x7F, 0x80, 0x7FF, 0x800, 0xFFFF, 0x10000, 0x10FFFF};
    char out[4];
    for (size_t i = 0; i < sizeof(test_cps) / sizeof(test_cps[0]); i++) {
        int len = utf8_encode(test_cps[i], out);
        TEST_ASSERT_TRUE(len >= 1 && len <= 4);
        const char* p = out;
        uint32_t decoded = utf8_decode(&p);
        TEST_ASSERT_EQUAL_INT(test_cps[i], decoded);
    }
}

/* ------------------------------------------------------------------ */
/*  utf8_codepoint_count tests                                           */
/* ------------------------------------------------------------------ */

void test_utf8_codepoint_count_ascii(void) {
    const char* s = "hello";
    TEST_ASSERT_EQUAL_INT(5, utf8_codepoint_count(s, 5));
}

void test_utf8_codepoint_count_mixed(void) {
    /* "中a文b" = 4 codepoints, 3+1+3+1 = 8 bytes */
    char buf[16];
    int pos = 0;
    pos += utf8_encode(0x4E2D, buf + pos);   /* 中 */
    pos += utf8_encode('a', buf + pos);
    pos += utf8_encode(0x6587, buf + pos);   /* 文 */
    pos += utf8_encode('b', buf + pos);
    buf[pos] = '\0';
    TEST_ASSERT_EQUAL_INT(4, utf8_codepoint_count(buf, pos));
}

/* ------------------------------------------------------------------ */
/*  utf8_cp_to_byte_offset tests                                         */
/* ------------------------------------------------------------------ */

void test_utf8_cp_to_byte_offset(void) {
    /* "a中b" */
    char buf[16];
    int pos = 0;
    pos += utf8_encode('a', buf + pos);
    pos += utf8_encode(0x4E2D, buf + pos);
    pos += utf8_encode('b', buf + pos);
    buf[pos] = '\0';

    TEST_ASSERT_EQUAL_INT(0, utf8_cp_to_byte_offset(buf, 0));
    TEST_ASSERT_EQUAL_INT(1, utf8_cp_to_byte_offset(buf, 1));  /* after 'a' */
    TEST_ASSERT_EQUAL_INT(4, utf8_cp_to_byte_offset(buf, 2));  /* after 中 */
    TEST_ASSERT_EQUAL_INT(5, utf8_cp_to_byte_offset(buf, 3));  /* after 'b' */
}

/* ------------------------------------------------------------------ */
/*  utf8_substring tests                                                 */
/* ------------------------------------------------------------------ */

void test_utf8_substring(void) {
    /* "a中b文c" */
    char buf[16];
    int pos = 0;
    pos += utf8_encode('a', buf + pos);
    pos += utf8_encode(0x4E2D, buf + pos);
    pos += utf8_encode('b', buf + pos);
    pos += utf8_encode(0x6587, buf + pos);
    pos += utf8_encode('c', buf + pos);
    buf[pos] = '\0';

    /* Extract "中b" (cp 1 to 3) */
    char* sub = utf8_substring(buf, 1, 3);
    TEST_ASSERT_NOT_NULL(sub);

    /* Verify it decodes to the right codepoints */
    const char* p = sub;
    TEST_ASSERT_EQUAL_INT(0x4E2D, utf8_decode(&p));
    TEST_ASSERT_EQUAL_INT('b', utf8_decode(&p));
    TEST_ASSERT_EQUAL_INT(0, *p);  /* null terminator */
    free(sub);
}

/* ------------------------------------------------------------------ */
/*  utf8_delete_range tests                                              */
/* ------------------------------------------------------------------ */

void test_utf8_delete_range(void) {
    /* "a中b文c" */
    char buf[32];
    int pos = 0;
    pos += utf8_encode('a', buf + pos);
    pos += utf8_encode(0x4E2D, buf + pos);
    pos += utf8_encode('b', buf + pos);
    pos += utf8_encode(0x6587, buf + pos);
    pos += utf8_encode('c', buf + pos);
    buf[pos] = '\0';
    int byte_len = pos;

    /* Delete "中b" (cp 1 to 3) => "a文c" */
    int new_len = utf8_delete_range(buf, byte_len, 1, 3);

    const char* p = buf;
    TEST_ASSERT_EQUAL_INT('a', utf8_decode(&p));
    TEST_ASSERT_EQUAL_INT(0x6587, utf8_decode(&p));
    TEST_ASSERT_EQUAL_INT('c', utf8_decode(&p));
    TEST_ASSERT_EQUAL_INT(0, *p);

    /* Verify byte count: 'a'(1) + 文(3) + 'c'(1) = 5 */
    TEST_ASSERT_EQUAL_INT(5, new_len);
}

/* ------------------------------------------------------------------ */
/*  utf8_insert tests                                                    */
/* ------------------------------------------------------------------ */

void test_utf8_insert(void) {
    char buf[64];
    int pos = 0;
    pos += utf8_encode('a', buf + pos);
    pos += utf8_encode('c', buf + pos);
    buf[pos] = '\0';
    int byte_len = pos;

    /* Insert "b" at cp_index 1 => "abc" */
    int new_len = utf8_insert(buf, byte_len, sizeof(buf), 1, "b");
    TEST_ASSERT_EQUAL_INT(3, new_len);
    TEST_ASSERT_EQUAL_STRING("abc", buf);
}

void test_utf8_insert_multibyte(void) {
    char buf[64];
    int pos = 0;
    pos += utf8_encode('a', buf + pos);
    pos += utf8_encode('b', buf + pos);
    buf[pos] = '\0';
    int byte_len = pos;

    /* Insert 中 at cp_index 1 => "a中b" */
    char inserted[4];
    int ilen = utf8_encode(0x4E2D, inserted);
    inserted[ilen] = '\0';
    int new_len = utf8_insert(buf, byte_len, sizeof(buf), 1, inserted);
    TEST_ASSERT_EQUAL_INT(5, new_len);  /* 1 + 3 + 1 */

    const char* p = buf;
    TEST_ASSERT_EQUAL_INT('a', utf8_decode(&p));
    TEST_ASSERT_EQUAL_INT(0x4E2D, utf8_decode(&p));
    TEST_ASSERT_EQUAL_INT('b', utf8_decode(&p));
}

void test_utf8_insert_overflow(void) {
    char buf[8];
    int pos = 0;
    pos += utf8_encode('a', buf + pos);
    buf[pos] = '\0';
    int byte_len = pos;

    /* Try to insert a string that would overflow the small buffer */
    int result = utf8_insert(buf, byte_len, 4, 0, "toolong");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

/* ------------------------------------------------------------------ */
/*  Display width tests                                                  */
/* ------------------------------------------------------------------ */

void test_utf8_display_width_cjk(void) {
    char buf[8];
    int pos = 0;
    pos += utf8_encode(0x4E2D, buf + pos);  /* 中 */
    pos += utf8_encode('a', buf + pos);
    buf[pos] = '\0';

    /* 中 should be 2 columns wide */
    TEST_ASSERT_EQUAL_INT(2, utf8_cp_display_width(buf, 0));
    /* 'a' should be 1 column wide */
    TEST_ASSERT_EQUAL_INT(1, utf8_cp_display_width(buf, 1));
}

void test_utf8_display_width_emoji(void) {
    char buf[8];
    int pos = 0;
    pos += utf8_encode(0x1F389, buf + pos);  /* party popper */
    buf[pos] = '\0';

    TEST_ASSERT_EQUAL_INT(2, utf8_cp_display_width(buf, 0));
}

void test_utf8_display_width_range(void) {
    /* "中a文" */
    char buf[16];
    int pos = 0;
    pos += utf8_encode(0x4E2D, buf + pos);
    pos += utf8_encode('a', buf + pos);
    pos += utf8_encode(0x6587, buf + pos);
    buf[pos] = '\0';

    /* Full width: 2 + 1 + 2 = 5 */
    TEST_ASSERT_EQUAL_INT(5, utf8_display_width(buf, 0, 3));
    /* Just "中a": 2 + 1 = 3 */
    TEST_ASSERT_EQUAL_INT(3, utf8_display_width(buf, 0, 2));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_utf8_encode_ascii);
    RUN_TEST(test_utf8_encode_2byte);
    RUN_TEST(test_utf8_encode_3byte);
    RUN_TEST(test_utf8_encode_4byte);
    RUN_TEST(test_utf8_decode_roundtrip);
    RUN_TEST(test_utf8_codepoint_count_ascii);
    RUN_TEST(test_utf8_codepoint_count_mixed);
    RUN_TEST(test_utf8_cp_to_byte_offset);
    RUN_TEST(test_utf8_substring);
    RUN_TEST(test_utf8_delete_range);
    RUN_TEST(test_utf8_insert);
    RUN_TEST(test_utf8_insert_multibyte);
    RUN_TEST(test_utf8_insert_overflow);
    RUN_TEST(test_utf8_display_width_cjk);
    RUN_TEST(test_utf8_display_width_emoji);
    RUN_TEST(test_utf8_display_width_range);

    return UNITY_END();
}
