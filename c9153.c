static void test_parse_uid(void) {
        int r;
        uid_t uid;

        log_info("/* %s */", __func__);

        r = parse_uid("100", &uid);
        assert_se(r == 0);
        assert_se(uid == 100);

        r = parse_uid("65535", &uid);
        assert_se(r == -ENXIO);

        r = parse_uid("asdsdas", &uid);
        assert_se(r == -EINVAL);
}