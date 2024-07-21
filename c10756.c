wl_map_insert_new(struct wl_map *map, uint32_t flags, void *data)
{
	union map_entry *start, *entry;
	struct wl_array *entries;
	uint32_t base;

	if (map->side == WL_MAP_CLIENT_SIDE) {
		entries = &map->client_entries;
		base = 0;
	} else {
		entries = &map->server_entries;
		base = WL_SERVER_ID_START;
	}

	if (map->free_list) {
		start = entries->data;
		entry = &start[map->free_list >> 1];
		map->free_list = entry->next;
	} else {
		entry = wl_array_add(entries, sizeof *entry);
		if (!entry)
			return 0;
		start = entries->data;
	}

	entry->data = data;
	entry->next |= (flags & 0x1) << 1;

	return (entry - start) + base;
}