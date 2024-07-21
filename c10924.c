static int map_delete_elem(union bpf_attr *attr)
{
	void __user *ukey = u64_to_user_ptr(attr->key);
	int ufd = attr->map_fd;
	struct bpf_map *map;
	struct fd f;
	void *key;
	int err;

	if (CHECK_ATTR(BPF_MAP_DELETE_ELEM))
		return -EINVAL;

	f = fdget(ufd);
	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);
	if (!(map_get_sys_perms(map, f) & FMODE_CAN_WRITE)) {
		err = -EPERM;
		goto err_put;
	}

	key = __bpf_copy_key(ukey, map->key_size);
	if (IS_ERR(key)) {
		err = PTR_ERR(key);
		goto err_put;
	}

	if (bpf_map_is_dev_bound(map)) {
		err = bpf_map_offload_delete_elem(map, key);
		goto out;
	} else if (IS_FD_PROG_ARRAY(map) ||
		   map->map_type == BPF_MAP_TYPE_STRUCT_OPS) {
		/* These maps require sleepable context */
		err = map->ops->map_delete_elem(map, key);
		goto out;
	}

	bpf_disable_instrumentation();
	rcu_read_lock();
	err = map->ops->map_delete_elem(map, key);
	rcu_read_unlock();
	bpf_enable_instrumentation();
	maybe_wait_bpf_programs(map);
out:
	kvfree(key);
err_put:
	fdput(f);
	return err;
}