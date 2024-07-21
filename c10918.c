static int bpf_map_do_batch(const union bpf_attr *attr,
			    union bpf_attr __user *uattr,
			    int cmd)
{
	struct bpf_map *map;
	int err, ufd;
	struct fd f;

	if (CHECK_ATTR(BPF_MAP_BATCH))
		return -EINVAL;

	ufd = attr->batch.map_fd;
	f = fdget(ufd);
	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);

	if ((cmd == BPF_MAP_LOOKUP_BATCH ||
	     cmd == BPF_MAP_LOOKUP_AND_DELETE_BATCH) &&
	    !(map_get_sys_perms(map, f) & FMODE_CAN_READ)) {
		err = -EPERM;
		goto err_put;
	}

	if (cmd != BPF_MAP_LOOKUP_BATCH &&
	    !(map_get_sys_perms(map, f) & FMODE_CAN_WRITE)) {
		err = -EPERM;
		goto err_put;
	}

	if (cmd == BPF_MAP_LOOKUP_BATCH)
		BPF_DO_BATCH(map->ops->map_lookup_batch);
	else if (cmd == BPF_MAP_LOOKUP_AND_DELETE_BATCH)
		BPF_DO_BATCH(map->ops->map_lookup_and_delete_batch);
	else if (cmd == BPF_MAP_UPDATE_BATCH)
		BPF_DO_BATCH(map->ops->map_update_batch);
	else
		BPF_DO_BATCH(map->ops->map_delete_batch);

err_put:
	fdput(f);
	return err;
}