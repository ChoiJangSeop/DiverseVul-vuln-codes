static void test_strnspn(void) {
        size_t len;

        len = c_shquote_strnspn(NULL, 0, "a");
        c_assert(len == 0);

        len = c_shquote_strnspn("a", 1, "");
        c_assert(len == 0);

        len = c_shquote_strnspn("ab", 2, "ac");
        c_assert(len == 1);

        len = c_shquote_strnspn("ab", 2, "bc");
        c_assert(len == 0);
}