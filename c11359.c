litest_utils_suite(void)
{
	TCase *tc;
	Suite *s;

	s = suite_create("litest:utils");
	tc = tcase_create("utils");

	tcase_add_test(tc, bitfield_helpers);
	tcase_add_test(tc, matrix_helpers);
	tcase_add_test(tc, ratelimit_helpers);
	tcase_add_test(tc, dpi_parser);
	tcase_add_test(tc, wheel_click_parser);
	tcase_add_test(tc, wheel_click_count_parser);
	tcase_add_test(tc, dimension_prop_parser);
	tcase_add_test(tc, reliability_prop_parser);
	tcase_add_test(tc, calibration_prop_parser);
	tcase_add_test(tc, range_prop_parser);
	tcase_add_test(tc, evcode_prop_parser);
	tcase_add_test(tc, input_prop_parser);
	tcase_add_test(tc, evdev_abs_parser);
	tcase_add_test(tc, safe_atoi_test);
	tcase_add_test(tc, safe_atoi_base_16_test);
	tcase_add_test(tc, safe_atoi_base_8_test);
	tcase_add_test(tc, safe_atou_test);
	tcase_add_test(tc, safe_atou_base_16_test);
	tcase_add_test(tc, safe_atou_base_8_test);
	tcase_add_test(tc, safe_atod_test);
	tcase_add_test(tc, strsplit_test);
	tcase_add_test(tc, strargv_test);
	tcase_add_test(tc, kvsplit_double_test);
	tcase_add_test(tc, strjoin_test);
	tcase_add_test(tc, strstrip_test);
	tcase_add_test(tc, strendswith_test);
	tcase_add_test(tc, strstartswith_test);
	tcase_add_test(tc, time_conversion);
	tcase_add_test(tc, human_time);

	tcase_add_test(tc, list_test_insert);
	tcase_add_test(tc, list_test_append);
	tcase_add_test(tc, strverscmp_test);
	tcase_add_test(tc, streq_test);
	tcase_add_test(tc, strneq_test);
	tcase_add_test(tc, trunkname_test);
	tcase_add_test(tc, basename_test);

	suite_add_tcase(s, tc);

	return s;
}