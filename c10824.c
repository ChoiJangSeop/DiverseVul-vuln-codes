struct fpm_scoreboard_proc_s *fpm_scoreboard_proc_get(struct fpm_scoreboard_s *scoreboard, int child_index) /* {{{*/
{
	if (!scoreboard) {
		scoreboard = fpm_scoreboard;
	}

	if (!scoreboard) {
		return NULL;
	}

	if (child_index < 0) {
		child_index = fpm_scoreboard_i;
	}

	if (child_index < 0 || (unsigned int)child_index >= scoreboard->nprocs) {
		return NULL;
	}

	return scoreboard->procs[child_index];
}