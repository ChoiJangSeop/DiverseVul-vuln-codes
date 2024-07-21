static int pcpu_need_to_extend(struct pcpu_chunk *chunk, bool is_atomic)
{
	int margin, new_alloc;

	if (is_atomic) {
		margin = 3;

		if (chunk->map_alloc <
		    chunk->map_used + PCPU_ATOMIC_MAP_MARGIN_LOW &&
		    pcpu_async_enabled)
			schedule_work(&chunk->map_extend_work);
	} else {
		margin = PCPU_ATOMIC_MAP_MARGIN_HIGH;
	}

	if (chunk->map_alloc >= chunk->map_used + margin)
		return 0;

	new_alloc = PCPU_DFL_MAP_ALLOC;
	while (new_alloc < chunk->map_used + margin)
		new_alloc *= 2;

	return new_alloc;
}