wl_map_reserve_new(struct wl_map *map, uint32_t i)
{
	union map_entry *start;
	uint32_t count;
	struct wl_array *entries;

	if (i < WL_SERVER_ID_START) {
		if (map->side == WL_MAP_CLIENT_SIDE) {
			errno = EINVAL;
			return -1;
		}

		entries = &map->client_entries;
	} else {
		if (map->side == WL_MAP_SERVER_SIDE) {
			errno = EINVAL;
			return -1;
		}

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

		start = entries->data;
		start[i].data = NULL;
	} else {
		start = entries->data;
		if (start[i].data != NULL) {
			errno = EINVAL;
			return -1;
		}
	}

	return 0;
}