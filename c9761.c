int __get_user_pages_fast(unsigned long start, int nr_pages, int write,
			  struct page **pages)
{
	unsigned long len, end;
	unsigned long flags;
	int nr_pinned = 0;
	/*
	 * Internally (within mm/gup.c), gup fast variants must set FOLL_GET,
	 * because gup fast is always a "pin with a +1 page refcount" request.
	 */
	unsigned int gup_flags = FOLL_GET;

	if (write)
		gup_flags |= FOLL_WRITE;

	start = untagged_addr(start) & PAGE_MASK;
	len = (unsigned long) nr_pages << PAGE_SHIFT;
	end = start + len;

	if (end <= start)
		return 0;
	if (unlikely(!access_ok((void __user *)start, len)))
		return 0;

	/*
	 * Disable interrupts.  We use the nested form as we can already have
	 * interrupts disabled by get_futex_key.
	 *
	 * With interrupts disabled, we block page table pages from being
	 * freed from under us. See struct mmu_table_batch comments in
	 * include/asm-generic/tlb.h for more details.
	 *
	 * We do not adopt an rcu_read_lock(.) here as we also want to
	 * block IPIs that come from THPs splitting.
	 */

	if (IS_ENABLED(CONFIG_HAVE_FAST_GUP) &&
	    gup_fast_permitted(start, end)) {
		local_irq_save(flags);
		gup_pgd_range(start, end, gup_flags, pages, &nr_pinned);
		local_irq_restore(flags);
	}

	return nr_pinned;
}