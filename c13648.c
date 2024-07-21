struct torture_suite *torture_raw_write(TALLOC_CTX *mem_ctx)
{
	struct torture_suite *suite = torture_suite_create(mem_ctx, "write");

	torture_suite_add_1smb_test(suite, "write", test_write);
	torture_suite_add_1smb_test(suite, "write unlock", test_writeunlock);
	torture_suite_add_1smb_test(suite, "write close", test_writeclose);
	torture_suite_add_1smb_test(suite, "writex", test_writex);

	return suite;
}