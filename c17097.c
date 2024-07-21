int setup_tests(void)
{
#ifndef OPENSSL_NO_DEPRECATED_3_0
    ADD_TEST(test_long);
#endif
    ADD_TEST(test_int32);
    ADD_TEST(test_uint32);
    ADD_TEST(test_int64);
    ADD_TEST(test_uint64);
    return 1;
}