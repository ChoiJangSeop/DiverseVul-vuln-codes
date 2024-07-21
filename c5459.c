static bool syn_check_config(const char *cfgstring, char **reason)
{
	tcmu_dbg("syn check config\n");
	if (strcmp(cfgstring, "syn/null")) {
		asprintf(reason, "invalid option");
		return false;
	}
	return true;
}