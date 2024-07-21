static void fpm_worker_pool_cleanup(int which, void *arg) /* {{{ */
{
	struct fpm_worker_pool_s *wp, *wp_next;

	for (wp = fpm_worker_all_pools; wp; wp = wp_next) {
		wp_next = wp->next;
		fpm_worker_pool_config_free(wp->config);
		fpm_children_free(wp->children);
		if ((which & FPM_CLEANUP_CHILD) == 0 && fpm_globals.parent_pid == getpid()) {
			fpm_scoreboard_free(wp->scoreboard);
		}
		fpm_worker_pool_free(wp);
	}
	fpm_worker_all_pools = NULL;
}