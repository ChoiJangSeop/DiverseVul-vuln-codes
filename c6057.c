static void __munlock_pagevec(struct pagevec *pvec, struct zone *zone)
{
	int i;
	int nr = pagevec_count(pvec);
	int delta_munlocked;
	struct pagevec pvec_putback;
	int pgrescued = 0;

	pagevec_init(&pvec_putback, 0);

	/* Phase 1: page isolation */
	spin_lock_irq(zone_lru_lock(zone));
	for (i = 0; i < nr; i++) {
		struct page *page = pvec->pages[i];

		if (TestClearPageMlocked(page)) {
			/*
			 * We already have pin from follow_page_mask()
			 * so we can spare the get_page() here.
			 */
			if (__munlock_isolate_lru_page(page, false))
				continue;
			else
				__munlock_isolation_failed(page);
		}

		/*
		 * We won't be munlocking this page in the next phase
		 * but we still need to release the follow_page_mask()
		 * pin. We cannot do it under lru_lock however. If it's
		 * the last pin, __page_cache_release() would deadlock.
		 */
		pagevec_add(&pvec_putback, pvec->pages[i]);
		pvec->pages[i] = NULL;
	}
	delta_munlocked = -nr + pagevec_count(&pvec_putback);
	__mod_zone_page_state(zone, NR_MLOCK, delta_munlocked);
	spin_unlock_irq(zone_lru_lock(zone));

	/* Now we can release pins of pages that we are not munlocking */
	pagevec_release(&pvec_putback);

	/* Phase 2: page munlock */
	for (i = 0; i < nr; i++) {
		struct page *page = pvec->pages[i];

		if (page) {
			lock_page(page);
			if (!__putback_lru_fast_prepare(page, &pvec_putback,
					&pgrescued)) {
				/*
				 * Slow path. We don't want to lose the last
				 * pin before unlock_page()
				 */
				get_page(page); /* for putback_lru_page() */
				__munlock_isolated_page(page);
				unlock_page(page);
				put_page(page); /* from follow_page_mask() */
			}
		}
	}

	/*
	 * Phase 3: page putback for pages that qualified for the fast path
	 * This will also call put_page() to return pin from follow_page_mask()
	 */
	if (pagevec_count(&pvec_putback))
		__putback_lru_fast(&pvec_putback, pgrescued);
}