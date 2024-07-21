static void test_syslog_parse_identifier(const char *str,
                                         const char *ident, const char *pid, int ret) {
        const char *buf = str;
        _cleanup_free_ char *ident2 = NULL, *pid2 = NULL;
        int ret2;

        ret2 = syslog_parse_identifier(&buf, &ident2, &pid2);

        assert_se(ret == ret2);
        assert_se(ident == ident2 || streq_ptr(ident, ident2));
        assert_se(pid == pid2 || streq_ptr(pid, pid2));
}