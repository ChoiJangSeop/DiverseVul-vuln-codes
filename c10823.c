int fpm_request_last_activity(struct fpm_child_s *child, struct timeval *tv) /* {{{ */
{
	struct fpm_scoreboard_proc_s *proc;

	if (!tv) return -1;

	proc = fpm_scoreboard_proc_get(child->wp->scoreboard, child->scoreboard_i);
	if (!proc) {
		return -1;
	}

	*tv = proc->tv;

	return 1;
}