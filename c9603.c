struct torture_suite *torture_smb2_notify_init(TALLOC_CTX *ctx)
{
	struct torture_suite *suite = torture_suite_create(ctx, "notify");

	torture_suite_add_1smb2_test(suite, "valid-req", test_valid_request);
	torture_suite_add_1smb2_test(suite, "tcon", torture_smb2_notify_tcon);
	torture_suite_add_2smb2_test(suite, "dir", torture_smb2_notify_dir);
	torture_suite_add_2smb2_test(suite, "mask", torture_smb2_notify_mask);
	torture_suite_add_1smb2_test(suite, "tdis", torture_smb2_notify_tree_disconnect);
	torture_suite_add_1smb2_test(suite, "tdis1", torture_smb2_notify_tree_disconnect_1);
	torture_suite_add_2smb2_test(suite, "mask-change", torture_smb2_notify_mask_change);
	torture_suite_add_1smb2_test(suite, "close", torture_smb2_notify_close);
	torture_suite_add_1smb2_test(suite, "logoff", torture_smb2_notify_ulogoff);
	torture_suite_add_1smb2_test(suite, "session-reconnect", torture_smb2_notify_session_reconnect);
	torture_suite_add_2smb2_test(suite, "invalid-reauth", torture_smb2_notify_invalid_reauth);
	torture_suite_add_1smb2_test(suite, "tree", torture_smb2_notify_tree);
	torture_suite_add_2smb2_test(suite, "basedir", torture_smb2_notify_basedir);
	torture_suite_add_2smb2_test(suite, "double", torture_smb2_notify_double);
	torture_suite_add_1smb2_test(suite, "file", torture_smb2_notify_file);
	torture_suite_add_1smb2_test(suite, "tcp", torture_smb2_notify_tcp_disconnect);
	torture_suite_add_2smb2_test(suite, "rec", torture_smb2_notify_recursive);
	torture_suite_add_1smb2_test(suite, "overflow", torture_smb2_notify_overflow);
	torture_suite_add_1smb2_test(suite, "rmdir1",
				     torture_smb2_notify_rmdir1);
	torture_suite_add_1smb2_test(suite, "rmdir2",
				     torture_smb2_notify_rmdir2);
	torture_suite_add_2smb2_test(suite, "rmdir3",
				     torture_smb2_notify_rmdir3);
	torture_suite_add_2smb2_test(suite, "rmdir4",
				     torture_smb2_notify_rmdir4);

	suite->description = talloc_strdup(suite, "SMB2-NOTIFY tests");

	return suite;
}