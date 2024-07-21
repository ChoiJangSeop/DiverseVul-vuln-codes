int ssh_scp_push_file(ssh_scp scp, const char *filename, size_t size, int mode){
	return ssh_scp_push_file64(scp, filename, (uint64_t) size, mode);
}