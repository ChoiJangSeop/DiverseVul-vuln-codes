void unix_notinflight(struct file *fp)
{
	struct sock *s = unix_get_socket(fp);

	if (s) {
		struct unix_sock *u = unix_sk(s);

		spin_lock(&unix_gc_lock);
		BUG_ON(list_empty(&u->link));

		if (atomic_long_dec_and_test(&u->inflight))
			list_del_init(&u->link);
		unix_tot_inflight--;
		spin_unlock(&unix_gc_lock);
	}
}