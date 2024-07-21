static void timer_stats_account_timer(struct timer_list *timer)
{
	void *site;

	/*
	 * start_site can be concurrently reset by
	 * timer_stats_timer_clear_start_info()
	 */
	site = READ_ONCE(timer->start_site);
	if (likely(!site))
		return;

	timer_stats_update_stats(timer, timer->start_pid, site,
				 timer->function, timer->start_comm,
				 timer->flags);
}