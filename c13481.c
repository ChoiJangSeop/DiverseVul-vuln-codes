static void test_strncspn(void) {
        size_t len;

        len = c_shquote_strncspn(NULL, 0, "a");
        c_assert(len == 0);

        len = c_shquote_strncspn("a", 1, "");
        c_assert(len == 1);

        len = c_shquote_strncspn("ab", 2, "ac");
        c_assert(len == 0);

        len = c_shquote_strncspn("ab", 2, "bc");
        c_assert(len == 1);

        len = c_shquote_strncspn("ab", 2, "cd");
        c_assert(len == 2);
}