static int map_lookup_and_delete_elem(union bpf_attr *attr)
{
	void __user *ukey = u64_to_user_ptr(attr->key);
	void __user *uvalue = u64_to_user_ptr(attr->value);
	int ufd = attr->map_fd;
	struct bpf_map *map;
	void *key, *value;
	u32 value_size;
	struct fd f;
	int err;

	if (CHECK_ATTR(BPF_MAP_LOOKUP_AND_DELETE_ELEM))
		return -EINVAL;

	if (attr->flags & ~BPF_F_LOCK)
		return -EINVAL;

	f = fdget(ufd);
	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);
	if (!(map_get_sys_perms(map, f) & FMODE_CAN_READ) ||
	    !(map_get_sys_perms(map, f) & FMODE_CAN_WRITE)) {
		err = -EPERM;
		goto err_put;
	}

	if (attr->flags &&
	    (map->map_type == BPF_MAP_TYPE_QUEUE ||
	     map->map_type == BPF_MAP_TYPE_STACK)) {
		err = -EINVAL;
		goto err_put;
	}

	if ((attr->flags & BPF_F_LOCK) &&
	    !map_value_has_spin_lock(map)) {
		err = -EINVAL;
		goto err_put;
	}

	key = __bpf_copy_key(ukey, map->key_size);
	if (IS_ERR(key)) {
		err = PTR_ERR(key);
		goto err_put;
	}

	value_size = bpf_map_value_size(map);

	err = -ENOMEM;
	value = kvmalloc(value_size, GFP_USER | __GFP_NOWARN);
	if (!value)
		goto free_key;

	err = -ENOTSUPP;
	if (map->map_type == BPF_MAP_TYPE_QUEUE ||
	    map->map_type == BPF_MAP_TYPE_STACK) {
		err = map->ops->map_pop_elem(map, value);
	} else if (map->map_type == BPF_MAP_TYPE_HASH ||
		   map->map_type == BPF_MAP_TYPE_PERCPU_HASH ||
		   map->map_type == BPF_MAP_TYPE_LRU_HASH ||
		   map->map_type == BPF_MAP_TYPE_LRU_PERCPU_HASH) {
		if (!bpf_map_is_dev_bound(map)) {
			bpf_disable_instrumentation();
			rcu_read_lock();
			err = map->ops->map_lookup_and_delete_elem(map, key, value, attr->flags);
			rcu_read_unlock();
			bpf_enable_instrumentation();
		}
	}

	if (err)
		goto free_value;

	if (copy_to_user(uvalue, value, value_size) != 0) {
		err = -EFAULT;
		goto free_value;
	}

	err = 0;

free_value:
	kvfree(value);
free_key:
	kvfree(key);
err_put:
	fdput(f);
	return err;
}