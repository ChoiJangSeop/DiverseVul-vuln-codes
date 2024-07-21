R_API RCmdAliasVal *r_cmd_alias_get(RCmd *cmd, const char *k) {
	r_return_val_if_fail (cmd && cmd->aliases && k, NULL);
	return ht_pp_find(cmd->aliases, k, NULL);
}