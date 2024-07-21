void string_conversion_test()
{
    std::cout << QUtil::int_to_string(16059) << std::endl
	      << QUtil::int_to_string(16059, 7) << std::endl
	      << QUtil::int_to_string(16059, -7) << std::endl
	      << QUtil::double_to_string(3.14159) << std::endl
	      << QUtil::double_to_string(3.14159, 3) << std::endl
	      << QUtil::double_to_string(1000.123, -1024) << std::endl
              << QUtil::double_to_string(.1234, 5) << std::endl
              << QUtil::double_to_string(.0001234, 5) << std::endl
              << QUtil::double_to_string(.123456, 5) << std::endl
              << QUtil::double_to_string(.000123456, 5) << std::endl
              << QUtil::int_to_string_base(16059, 10) << std::endl
              << QUtil::int_to_string_base(16059, 8) << std::endl
              << QUtil::int_to_string_base(16059, 16) << std::endl;

    std::string embedded_null = "one";
    embedded_null += '\0';
    embedded_null += "two";
    std::cout << embedded_null.c_str() << std::endl;
    std::cout << embedded_null.length() << std::endl;
    char* tmp = QUtil::copy_string(embedded_null);
    if (memcmp(tmp, embedded_null.c_str(), 7) == 0)
    {
	std::cout << "compare okay" << std::endl;
    }
    else
    {
	std::cout << "compare failed" << std::endl;
    }
    delete [] tmp;

    std::string int_max_str = QUtil::int_to_string(INT_MAX);
    std::string int_min_str = QUtil::int_to_string(INT_MIN);
    long long int_max_plus_1 = static_cast<long long>(INT_MAX) + 1;
    long long int_min_minus_1 = static_cast<long long>(INT_MIN) - 1;
    std::string int_max_plus_1_str = QUtil::int_to_string(int_max_plus_1);
    std::string int_min_minus_1_str = QUtil::int_to_string(int_min_minus_1);
    std::string small_positive = QUtil::uint_to_string(16059U);
    std::string small_negative = QUtil::int_to_string(-16059);
    test_to_int(int_min_str.c_str(), INT_MIN, false);
    test_to_int(int_max_str.c_str(), INT_MAX, false);
    test_to_int(int_max_plus_1_str.c_str(), 0, true);
    test_to_int(int_min_minus_1_str.c_str(), 0, true);
    test_to_int("9999999999999999999999999", 0, true);
    test_to_ll(int_max_plus_1_str.c_str(), int_max_plus_1, false);
    test_to_ll(int_min_minus_1_str.c_str(), int_min_minus_1, false);
    test_to_ll("99999999999999999999999999999999999999999999999999", 0, true);
    test_to_uint(small_positive.c_str(), 16059U, false);
    test_to_uint(small_negative.c_str(), 0, true);
    test_to_uint("9999999999", 0, true);
    test_to_ull(small_positive.c_str(), 16059U, false);
    test_to_ull(small_negative.c_str(), 0, true);
}