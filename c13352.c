TEST_F(EncryptionUtilTest, sm4_test_by_case) {
    std::string case_1 = "P/Ub8/arZ22TW+rAT5sgYg=="; // base64 for encrypted "hello, doris"
    std::string source_1 = "hello, doris";
    std::string case_2 = "2I+UW9axOP2Tv35BGYgy+g=="; // base64 for encrypted "doris test"
    std::string source_2 = "doris test";

    std::unique_ptr<char[]> encrypt_1(new char[case_1.length()]);
    int length_1 = base64_decode(case_1.c_str(), case_1.length(), encrypt_1.get());
    std::cout << encrypt_1.get();
    std::unique_ptr<char[]> decrypted_1(new char[case_1.length()]);
    int ret_code = EncryptionUtil::decrypt(SM4_128_ECB, (unsigned char*)encrypt_1.get(), length_1,
                                           (unsigned char*)_aes_key.c_str(), _aes_key.length(),
                                           nullptr, true, (unsigned char*)decrypted_1.get());
    ASSERT_TRUE(ret_code > 0);
    std::string decrypted_content_1(decrypted_1.get(), ret_code);
    ASSERT_EQ(source_1, decrypted_content_1);

    std::unique_ptr<char[]> encrypt_2(new char[case_2.length()]);
    int length_2 = base64_decode(case_2.c_str(), case_2.length(), encrypt_2.get());
    std::unique_ptr<char[]> decrypted_2(new char[case_2.length()]);
    ret_code = EncryptionUtil::decrypt(SM4_128_ECB, (unsigned char*)encrypt_2.get(), length_2,
                                       (unsigned char*)_aes_key.c_str(), _aes_key.length(), nullptr,
                                       true, (unsigned char*)decrypted_2.get());
    ASSERT_TRUE(ret_code > 0);
    std::string decrypted_content_2(decrypted_2.get(), ret_code);
    ASSERT_EQ(source_2, decrypted_content_2);
}