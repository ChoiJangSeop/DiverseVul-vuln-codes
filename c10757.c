wl_map_insert_at(struct wl_map *map, uint32_t flags, uint32_t i, void *data)
{
	union map_entry *start;
	uint32_t count;
	struct wl_array *entries;

	if (i < WL_SERVER_ID_START) {
		entries = &map->client_entries;
	} else {
		entries = &map->server_entries;
		i -= WL_SERVER_ID_START;
	}

	count = entries->size / sizeof *start;
	if (count < i) {
		errno = EINVAL;
		return -1;
	}

	if (count == i) {
		if (!wl_array_add(entries, sizeof *start))
			return -1;
	}

	start = entries->data;
	start[i].data = data;
	start[i].next |= (flags & 0x1) << 1;

	return 0;
}