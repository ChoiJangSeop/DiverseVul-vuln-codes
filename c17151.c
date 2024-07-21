int main(void)
{
	static void (*test_functions[])(void) = {
		test_do_init,
		test_jwt_hs_token,
		test_jwt_bad_valid_token,
		test_jwt_broken_token,
		test_jwt_dates,
		test_jwt_key_files,
		test_jwt_kid_escape,
		test_jwt_rs_token,
		test_jwt_ps_token,
		test_jwt_ec_token,
		test_do_deinit,
		NULL
	};
	int ret;
	ret = test_run(test_functions);
	return ret;
}