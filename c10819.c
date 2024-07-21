static void fpm_resources_discard(struct fpm_child_s *child) /* {{{ */
{
	fpm_scoreboard_proc_free(child->wp->scoreboard, child->scoreboard_i);
	fpm_stdio_discard_pipes(child);
	fpm_child_free(child);
}