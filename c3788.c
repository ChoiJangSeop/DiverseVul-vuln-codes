static void pcpu_map_extend_workfn(struct work_struct *work)
{
	struct pcpu_chunk *chunk = container_of(work, struct pcpu_chunk,
						map_extend_work);
	int new_alloc;

	spin_lock_irq(&pcpu_lock);
	new_alloc = pcpu_need_to_extend(chunk, false);
	spin_unlock_irq(&pcpu_lock);

	if (new_alloc)
		pcpu_extend_area_map(chunk, new_alloc);
}