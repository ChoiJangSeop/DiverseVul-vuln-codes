static void __attribute__((constructor)) init(void)
{
	g_test_add_func("/utils/parse_bool", test_parse_bool);
	g_test_add_func("/utils/die", test_die);
	g_test_add_func("/utils/die_with_errno", test_die_with_errno);
	g_test_add_func("/utils/sc_nonfatal_mkpath/relative",
			test_sc_nonfatal_mkpath__relative);
	g_test_add_func("/utils/sc_nonfatal_mkpath/absolute",
			test_sc_nonfatal_mkpath__absolute);
}