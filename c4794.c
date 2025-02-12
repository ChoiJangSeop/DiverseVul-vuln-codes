static void expire_timers(struct timer_base *base, struct hlist_head *head)
{
	while (!hlist_empty(head)) {
		struct timer_list *timer;
		void (*fn)(unsigned long);
		unsigned long data;

		timer = hlist_entry(head->first, struct timer_list, entry);
		timer_stats_account_timer(timer);

		base->running_timer = timer;
		detach_timer(timer, true);

		fn = timer->function;
		data = timer->data;

		if (timer->flags & TIMER_IRQSAFE) {
			spin_unlock(&base->lock);
			call_timer_fn(timer, fn, data);
			spin_lock(&base->lock);
		} else {
			spin_unlock_irq(&base->lock);
			call_timer_fn(timer, fn, data);
			spin_lock_irq(&base->lock);
		}
	}
}