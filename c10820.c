void fpm_scoreboard_free(struct fpm_scoreboard_s *scoreboard) /* {{{ */
{
	size_t scoreboard_size, scoreboard_nprocs_size;

	if (!scoreboard) {
		zlog(ZLOG_ERROR, "**scoreboard is NULL");
		return;
	}

	scoreboard_size        = sizeof(struct fpm_scoreboard_s) + (scoreboard->nprocs) * sizeof(struct fpm_scoreboard_proc_s *);
	scoreboard_nprocs_size = sizeof(struct fpm_scoreboard_proc_s) * scoreboard->nprocs;

	fpm_shm_free(scoreboard, scoreboard_size + scoreboard_nprocs_size);
}