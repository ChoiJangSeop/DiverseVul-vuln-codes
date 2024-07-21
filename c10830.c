int fpm_scoreboard_proc_alloc(struct fpm_scoreboard_s *scoreboard, int *child_index) /* {{{ */
{
	int i = -1;

	if (!scoreboard || !child_index) {
		return -1;
	}

	/* first try the slot which is supposed to be free */
	if (scoreboard->free_proc >= 0 && (unsigned int)scoreboard->free_proc < scoreboard->nprocs) {
		if (scoreboard->procs[scoreboard->free_proc] && !scoreboard->procs[scoreboard->free_proc]->used) {
			i = scoreboard->free_proc;
		}
	}

	if (i < 0) { /* the supposed free slot is not, let's search for a free slot */
		zlog(ZLOG_DEBUG, "[pool %s] the proc->free_slot was not free. Let's search", scoreboard->pool);
		for (i = 0; i < (int)scoreboard->nprocs; i++) {
			if (scoreboard->procs[i] && !scoreboard->procs[i]->used) { /* found */
				break;
			}
		}
	}

	/* no free slot */
	if (i < 0 || i >= (int)scoreboard->nprocs) {
		zlog(ZLOG_ERROR, "[pool %s] no free scoreboard slot", scoreboard->pool);
		return -1;
	}

	scoreboard->procs[i]->used = 1;
	*child_index = i;

	/* supposed next slot is free */
	if (i + 1 >= (int)scoreboard->nprocs) {
		scoreboard->free_proc = 0;
	} else {
		scoreboard->free_proc = i + 1;
	}

	return 0;
}