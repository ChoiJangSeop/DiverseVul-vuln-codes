static int ep_loop_check_proc(void *priv, void *cookie, int call_nests)
{
	int error = 0;
	struct file *file = priv;
	struct eventpoll *ep = file->private_data;
	struct rb_node *rbp;
	struct epitem *epi;

	mutex_lock_nested(&ep->mtx, call_nests + 1);
	for (rbp = rb_first(&ep->rbr); rbp; rbp = rb_next(rbp)) {
		epi = rb_entry(rbp, struct epitem, rbn);
		if (unlikely(is_file_epoll(epi->ffd.file))) {
			error = ep_call_nested(&poll_loop_ncalls, EP_MAX_NESTS,
					       ep_loop_check_proc, epi->ffd.file,
					       epi->ffd.file->private_data, current);
			if (error != 0)
				break;
		}
	}
	mutex_unlock(&ep->mtx);

	return error;
}