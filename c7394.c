int memory_failure(unsigned long pfn, int trapno, int flags)
{
	struct page_state *ps;
	struct page *p;
	struct page *hpage;
	struct page *orig_head;
	int res;
	unsigned int nr_pages;
	unsigned long page_flags;

	if (!sysctl_memory_failure_recovery)
		panic("Memory failure from trap %d on page %lx", trapno, pfn);

	if (!pfn_valid(pfn)) {
		pr_err("Memory failure: %#lx: memory outside kernel control\n",
			pfn);
		return -ENXIO;
	}

	p = pfn_to_page(pfn);
	orig_head = hpage = compound_head(p);
	if (TestSetPageHWPoison(p)) {
		pr_err("Memory failure: %#lx: already hardware poisoned\n",
			pfn);
		return 0;
	}

	/*
	 * Currently errors on hugetlbfs pages are measured in hugepage units,
	 * so nr_pages should be 1 << compound_order.  OTOH when errors are on
	 * transparent hugepages, they are supposed to be split and error
	 * measurement is done in normal page units.  So nr_pages should be one
	 * in this case.
	 */
	if (PageHuge(p))
		nr_pages = 1 << compound_order(hpage);
	else /* normal page or thp */
		nr_pages = 1;
	num_poisoned_pages_add(nr_pages);

	/*
	 * We need/can do nothing about count=0 pages.
	 * 1) it's a free page, and therefore in safe hand:
	 *    prep_new_page() will be the gate keeper.
	 * 2) it's a free hugepage, which is also safe:
	 *    an affected hugepage will be dequeued from hugepage freelist,
	 *    so there's no concern about reusing it ever after.
	 * 3) it's part of a non-compound high order page.
	 *    Implies some kernel user: cannot stop them from
	 *    R/W the page; let's pray that the page has been
	 *    used and will be freed some time later.
	 * In fact it's dangerous to directly bump up page count from 0,
	 * that may make page_freeze_refs()/page_unfreeze_refs() mismatch.
	 */
	if (!(flags & MF_COUNT_INCREASED) && !get_hwpoison_page(p)) {
		if (is_free_buddy_page(p)) {
			action_result(pfn, MF_MSG_BUDDY, MF_DELAYED);
			return 0;
		} else if (PageHuge(hpage)) {
			/*
			 * Check "filter hit" and "race with other subpage."
			 */
			lock_page(hpage);
			if (PageHWPoison(hpage)) {
				if ((hwpoison_filter(p) && TestClearPageHWPoison(p))
				    || (p != hpage && TestSetPageHWPoison(hpage))) {
					num_poisoned_pages_sub(nr_pages);
					unlock_page(hpage);
					return 0;
				}
			}
			set_page_hwpoison_huge_page(hpage);
			res = dequeue_hwpoisoned_huge_page(hpage);
			action_result(pfn, MF_MSG_FREE_HUGE,
				      res ? MF_IGNORED : MF_DELAYED);
			unlock_page(hpage);
			return res;
		} else {
			action_result(pfn, MF_MSG_KERNEL_HIGH_ORDER, MF_IGNORED);
			return -EBUSY;
		}
	}

	if (!PageHuge(p) && PageTransHuge(hpage)) {
		lock_page(hpage);
		if (!PageAnon(hpage) || unlikely(split_huge_page(hpage))) {
			unlock_page(hpage);
			if (!PageAnon(hpage))
				pr_err("Memory failure: %#lx: non anonymous thp\n",
					pfn);
			else
				pr_err("Memory failure: %#lx: thp split failed\n",
					pfn);
			if (TestClearPageHWPoison(p))
				num_poisoned_pages_sub(nr_pages);
			put_hwpoison_page(p);
			return -EBUSY;
		}
		unlock_page(hpage);
		get_hwpoison_page(p);
		put_hwpoison_page(hpage);
		VM_BUG_ON_PAGE(!page_count(p), p);
		hpage = compound_head(p);
	}

	/*
	 * We ignore non-LRU pages for good reasons.
	 * - PG_locked is only well defined for LRU pages and a few others
	 * - to avoid races with __SetPageLocked()
	 * - to avoid races with __SetPageSlab*() (and more non-atomic ops)
	 * The check (unnecessarily) ignores LRU pages being isolated and
	 * walked by the page reclaim code, however that's not a big loss.
	 */
	if (!PageHuge(p)) {
		if (!PageLRU(p))
			shake_page(p, 0);
		if (!PageLRU(p)) {
			/*
			 * shake_page could have turned it free.
			 */
			if (is_free_buddy_page(p)) {
				if (flags & MF_COUNT_INCREASED)
					action_result(pfn, MF_MSG_BUDDY, MF_DELAYED);
				else
					action_result(pfn, MF_MSG_BUDDY_2ND,
						      MF_DELAYED);
				return 0;
			}
		}
	}

	lock_page(hpage);

	/*
	 * The page could have changed compound pages during the locking.
	 * If this happens just bail out.
	 */
	if (PageCompound(p) && compound_head(p) != orig_head) {
		action_result(pfn, MF_MSG_DIFFERENT_COMPOUND, MF_IGNORED);
		res = -EBUSY;
		goto out;
	}

	/*
	 * We use page flags to determine what action should be taken, but
	 * the flags can be modified by the error containment action.  One
	 * example is an mlocked page, where PG_mlocked is cleared by
	 * page_remove_rmap() in try_to_unmap_one(). So to determine page status
	 * correctly, we save a copy of the page flags at this time.
	 */
	page_flags = p->flags;

	/*
	 * unpoison always clear PG_hwpoison inside page lock
	 */
	if (!PageHWPoison(p)) {
		pr_err("Memory failure: %#lx: just unpoisoned\n", pfn);
		num_poisoned_pages_sub(nr_pages);
		unlock_page(hpage);
		put_hwpoison_page(hpage);
		return 0;
	}
	if (hwpoison_filter(p)) {
		if (TestClearPageHWPoison(p))
			num_poisoned_pages_sub(nr_pages);
		unlock_page(hpage);
		put_hwpoison_page(hpage);
		return 0;
	}

	if (!PageHuge(p) && !PageTransTail(p) && !PageLRU(p))
		goto identify_page_state;

	/*
	 * For error on the tail page, we should set PG_hwpoison
	 * on the head page to show that the hugepage is hwpoisoned
	 */
	if (PageHuge(p) && PageTail(p) && TestSetPageHWPoison(hpage)) {
		action_result(pfn, MF_MSG_POISONED_HUGE, MF_IGNORED);
		unlock_page(hpage);
		put_hwpoison_page(hpage);
		return 0;
	}
	/*
	 * Set PG_hwpoison on all pages in an error hugepage,
	 * because containment is done in hugepage unit for now.
	 * Since we have done TestSetPageHWPoison() for the head page with
	 * page lock held, we can safely set PG_hwpoison bits on tail pages.
	 */
	if (PageHuge(p))
		set_page_hwpoison_huge_page(hpage);

	/*
	 * It's very difficult to mess with pages currently under IO
	 * and in many cases impossible, so we just avoid it here.
	 */
	wait_on_page_writeback(p);

	/*
	 * Now take care of user space mappings.
	 * Abort on fail: __delete_from_page_cache() assumes unmapped page.
	 *
	 * When the raw error page is thp tail page, hpage points to the raw
	 * page after thp split.
	 */
	if (hwpoison_user_mappings(p, pfn, trapno, flags, &hpage)
	    != SWAP_SUCCESS) {
		action_result(pfn, MF_MSG_UNMAP_FAILED, MF_IGNORED);
		res = -EBUSY;
		goto out;
	}

	/*
	 * Torn down by someone else?
	 */
	if (PageLRU(p) && !PageSwapCache(p) && p->mapping == NULL) {
		action_result(pfn, MF_MSG_TRUNCATED_LRU, MF_IGNORED);
		res = -EBUSY;
		goto out;
	}

identify_page_state:
	res = -EBUSY;
	/*
	 * The first check uses the current page flags which may not have any
	 * relevant information. The second check with the saved page flagss is
	 * carried out only if the first check can't determine the page status.
	 */
	for (ps = error_states;; ps++)
		if ((p->flags & ps->mask) == ps->res)
			break;

	page_flags |= (p->flags & (1UL << PG_dirty));

	if (!ps->mask)
		for (ps = error_states;; ps++)
			if ((page_flags & ps->mask) == ps->res)
				break;
	res = page_action(ps, p, pfn);
out:
	unlock_page(hpage);
	return res;
}