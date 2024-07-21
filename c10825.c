int fpm_request_is_idle(struct fpm_child_s *child) /* {{{ */
{
	struct fpm_scoreboard_proc_s *proc;

	/* no need in atomicity here */
	proc = fpm_scoreboard_proc_get(child->wp->scoreboard, child->scoreboard_i);
	if (!proc) {
		return 0;
	}

	return proc->request_stage == FPM_REQUEST_ACCEPTING;
}