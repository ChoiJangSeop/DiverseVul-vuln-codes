struct torture_suite *torture_winbind_struct_init(TALLOC_CTX *ctx)
{
	struct torture_suite *suite = torture_suite_create(ctx, "struct");

	torture_suite_add_simple_test(suite, "interface_version", torture_winbind_struct_interface_version);
	torture_suite_add_simple_test(suite, "ping", torture_winbind_struct_ping);
	torture_suite_add_simple_test(suite, "info", torture_winbind_struct_info);
	torture_suite_add_simple_test(suite, "priv_pipe_dir", torture_winbind_struct_priv_pipe_dir);
	torture_suite_add_simple_test(suite, "netbios_name", torture_winbind_struct_netbios_name);
	torture_suite_add_simple_test(suite, "domain_name", torture_winbind_struct_domain_name);
	torture_suite_add_simple_test(suite, "check_machacc", torture_winbind_struct_check_machacc);
	torture_suite_add_simple_test(suite, "list_trustdom", torture_winbind_struct_list_trustdom);
	torture_suite_add_simple_test(suite, "domain_info", torture_winbind_struct_domain_info);
	torture_suite_add_simple_test(suite, "getdcname", torture_winbind_struct_getdcname);
	torture_suite_add_simple_test(suite, "dsgetdcname", torture_winbind_struct_dsgetdcname);
	torture_suite_add_simple_test(suite, "list_users", torture_winbind_struct_list_users);
	torture_suite_add_simple_test(suite, "list_groups", torture_winbind_struct_list_groups);
	torture_suite_add_simple_test(suite, "show_sequence", torture_winbind_struct_show_sequence);
	torture_suite_add_simple_test(suite, "setpwent", torture_winbind_struct_setpwent);
	torture_suite_add_simple_test(suite, "getpwent", torture_winbind_struct_getpwent);
	torture_suite_add_simple_test(suite, "endpwent", torture_winbind_struct_endpwent);
	torture_suite_add_simple_test(suite, "lookup_name_sid", torture_winbind_struct_lookup_name_sid);

	suite->description = talloc_strdup(suite, "WINBIND - struct based protocol tests");

	return suite;
}