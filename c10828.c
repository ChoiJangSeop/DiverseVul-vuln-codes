static void fpm_child_resources_use(struct fpm_child_s *child) /* {{{ */
{
	struct fpm_worker_pool_s *wp;
	for (wp = fpm_worker_all_pools; wp; wp = wp->next) {
		if (wp == child->wp || wp == child->wp->shared) {
			continue;
		}
		fpm_scoreboard_free(wp->scoreboard);
	}

	fpm_scoreboard_child_use(child->wp->scoreboard, child->scoreboard_i, getpid());
	fpm_stdio_child_use_pipes(child);
	fpm_child_free(child);
}