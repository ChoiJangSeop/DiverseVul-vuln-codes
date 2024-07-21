static int ep_loop_check(struct eventpoll *ep, struct file *file)
{
	return ep_call_nested(&poll_loop_ncalls, EP_MAX_NESTS,
			      ep_loop_check_proc, file, ep, current);
}