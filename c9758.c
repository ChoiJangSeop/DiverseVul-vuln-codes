static int internal_get_user_pages_fast(unsigned long start, int nr_pages,
					unsigned int gup_flags,
					struct page **pages)
{
	unsigned long addr, len, end;
	int nr_pinned = 0, ret = 0;

	if (WARN_ON_ONCE(gup_flags & ~(FOLL_WRITE | FOLL_LONGTERM |
				       FOLL_FORCE | FOLL_PIN | FOLL_GET)))
		return -EINVAL;

	start = untagged_addr(start) & PAGE_MASK;
	addr = start;
	len = (unsigned long) nr_pages << PAGE_SHIFT;
	end = start + len;

	if (end <= start)
		return 0;
	if (unlikely(!access_ok((void __user *)start, len)))
		return -EFAULT;

	if (IS_ENABLED(CONFIG_HAVE_FAST_GUP) &&
	    gup_fast_permitted(start, end)) {
		local_irq_disable();
		gup_pgd_range(addr, end, gup_flags, pages, &nr_pinned);
		local_irq_enable();
		ret = nr_pinned;
	}

	if (nr_pinned < nr_pages) {
		/* Try to get the remaining pages with get_user_pages */
		start += nr_pinned << PAGE_SHIFT;
		pages += nr_pinned;

		ret = __gup_longterm_unlocked(start, nr_pages - nr_pinned,
					      gup_flags, pages);

		/* Have to be a bit careful with return values */
		if (nr_pinned > 0) {
			if (ret < 0)
				ret = nr_pinned;
			else
				ret += nr_pinned;
		}
	}

	return ret;
}