R_API int r_sys_cmd_str_full(const char *cmd, const char *input, char **output, int *len, char **sterr) {
	return r_sys_cmd_str_full_w32 (cmd, input, output, len, sterr);
}