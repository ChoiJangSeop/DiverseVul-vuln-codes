static void pcpu_balance_workfn(struct work_struct *work)
{
	LIST_HEAD(to_free);
	struct list_head *free_head = &pcpu_slot[pcpu_nr_slots - 1];
	struct pcpu_chunk *chunk, *next;
	int slot, nr_to_pop, ret;

	/*
	 * There's no reason to keep around multiple unused chunks and VM
	 * areas can be scarce.  Destroy all free chunks except for one.
	 */
	mutex_lock(&pcpu_alloc_mutex);
	spin_lock_irq(&pcpu_lock);

	list_for_each_entry_safe(chunk, next, free_head, list) {
		WARN_ON(chunk->immutable);

		/* spare the first one */
		if (chunk == list_first_entry(free_head, struct pcpu_chunk, list))
			continue;

		list_move(&chunk->list, &to_free);
	}

	spin_unlock_irq(&pcpu_lock);

	list_for_each_entry_safe(chunk, next, &to_free, list) {
		int rs, re;

		pcpu_for_each_pop_region(chunk, rs, re, 0, pcpu_unit_pages) {
			pcpu_depopulate_chunk(chunk, rs, re);
			spin_lock_irq(&pcpu_lock);
			pcpu_chunk_depopulated(chunk, rs, re);
			spin_unlock_irq(&pcpu_lock);
		}
		pcpu_destroy_chunk(chunk);
	}

	/*
	 * Ensure there are certain number of free populated pages for
	 * atomic allocs.  Fill up from the most packed so that atomic
	 * allocs don't increase fragmentation.  If atomic allocation
	 * failed previously, always populate the maximum amount.  This
	 * should prevent atomic allocs larger than PAGE_SIZE from keeping
	 * failing indefinitely; however, large atomic allocs are not
	 * something we support properly and can be highly unreliable and
	 * inefficient.
	 */
retry_pop:
	if (pcpu_atomic_alloc_failed) {
		nr_to_pop = PCPU_EMPTY_POP_PAGES_HIGH;
		/* best effort anyway, don't worry about synchronization */
		pcpu_atomic_alloc_failed = false;
	} else {
		nr_to_pop = clamp(PCPU_EMPTY_POP_PAGES_HIGH -
				  pcpu_nr_empty_pop_pages,
				  0, PCPU_EMPTY_POP_PAGES_HIGH);
	}

	for (slot = pcpu_size_to_slot(PAGE_SIZE); slot < pcpu_nr_slots; slot++) {
		int nr_unpop = 0, rs, re;

		if (!nr_to_pop)
			break;

		spin_lock_irq(&pcpu_lock);
		list_for_each_entry(chunk, &pcpu_slot[slot], list) {
			nr_unpop = pcpu_unit_pages - chunk->nr_populated;
			if (nr_unpop)
				break;
		}
		spin_unlock_irq(&pcpu_lock);

		if (!nr_unpop)
			continue;

		/* @chunk can't go away while pcpu_alloc_mutex is held */
		pcpu_for_each_unpop_region(chunk, rs, re, 0, pcpu_unit_pages) {
			int nr = min(re - rs, nr_to_pop);

			ret = pcpu_populate_chunk(chunk, rs, rs + nr);
			if (!ret) {
				nr_to_pop -= nr;
				spin_lock_irq(&pcpu_lock);
				pcpu_chunk_populated(chunk, rs, rs + nr);
				spin_unlock_irq(&pcpu_lock);
			} else {
				nr_to_pop = 0;
			}

			if (!nr_to_pop)
				break;
		}
	}

	if (nr_to_pop) {
		/* ran out of chunks to populate, create a new one and retry */
		chunk = pcpu_create_chunk();
		if (chunk) {
			spin_lock_irq(&pcpu_lock);
			pcpu_chunk_relocate(chunk, -1);
			spin_unlock_irq(&pcpu_lock);
			goto retry_pop;
		}
	}

	mutex_unlock(&pcpu_alloc_mutex);
}