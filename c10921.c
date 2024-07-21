static int map_freeze(const union bpf_attr *attr)
{
	int err = 0, ufd = attr->map_fd;
	struct bpf_map *map;
	struct fd f;

	if (CHECK_ATTR(BPF_MAP_FREEZE))
		return -EINVAL;

	f = fdget(ufd);
	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);

	if (map->map_type == BPF_MAP_TYPE_STRUCT_OPS ||
	    map_value_has_timer(map)) {
		fdput(f);
		return -ENOTSUPP;
	}

	mutex_lock(&map->freeze_mutex);

	if (map->writecnt) {
		err = -EBUSY;
		goto err_put;
	}
	if (READ_ONCE(map->frozen)) {
		err = -EBUSY;
		goto err_put;
	}
	if (!bpf_capable()) {
		err = -EPERM;
		goto err_put;
	}

	WRITE_ONCE(map->frozen, true);
err_put:
	mutex_unlock(&map->freeze_mutex);
	fdput(f);
	return err;
}