static int pcpu_extend_area_map(struct pcpu_chunk *chunk, int new_alloc)
{
	int *old = NULL, *new = NULL;
	size_t old_size = 0, new_size = new_alloc * sizeof(new[0]);
	unsigned long flags;

	new = pcpu_mem_zalloc(new_size);
	if (!new)
		return -ENOMEM;

	/* acquire pcpu_lock and switch to new area map */
	spin_lock_irqsave(&pcpu_lock, flags);

	if (new_alloc <= chunk->map_alloc)
		goto out_unlock;

	old_size = chunk->map_alloc * sizeof(chunk->map[0]);
	old = chunk->map;

	memcpy(new, old, old_size);

	chunk->map_alloc = new_alloc;
	chunk->map = new;
	new = NULL;

out_unlock:
	spin_unlock_irqrestore(&pcpu_lock, flags);

	/*
	 * pcpu_mem_free() might end up calling vfree() which uses
	 * IRQ-unsafe lock and thus can't be called under pcpu_lock.
	 */
	pcpu_mem_free(old);
	pcpu_mem_free(new);

	return 0;
}