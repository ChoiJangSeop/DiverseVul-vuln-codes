void fpm_scoreboard_proc_free(struct fpm_scoreboard_s *scoreboard, int child_index) /* {{{ */
{
	if (!scoreboard) {
		return;
	}

	if (child_index < 0 || (unsigned int)child_index >= scoreboard->nprocs) {
		return;
	}

	if (scoreboard->procs[child_index] && scoreboard->procs[child_index]->used > 0) {
		memset(scoreboard->procs[child_index], 0, sizeof(struct fpm_scoreboard_proc_s));
	}

	/* set this slot as free to avoid search on next alloc */
	scoreboard->free_proc = child_index;
}