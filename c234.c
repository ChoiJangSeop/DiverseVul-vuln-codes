void unix_notinflight(struct file *fp)
{
	struct sock *s = unix_get_socket(fp);
	if(s) {
		atomic_dec(&unix_sk(s)->inflight);
		atomic_dec(&unix_tot_inflight);
	}
}