int ssh_scp_integer_mode(const char *mode){
	int value=strtoul(mode,NULL,8) & 0xffff;
	return value;
}