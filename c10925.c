static int map_update_elem(union bpf_attr *attr, bpfptr_t uattr)
{
	bpfptr_t ukey = make_bpfptr(attr->key, uattr.is_kernel);
	bpfptr_t uvalue = make_bpfptr(attr->value, uattr.is_kernel);
	int ufd = attr->map_fd;
	struct bpf_map *map;
	void *key, *value;
	u32 value_size;
	struct fd f;
	int err;

	if (CHECK_ATTR(BPF_MAP_UPDATE_ELEM))
		return -EINVAL;

	f = fdget(ufd);
	map = __bpf_map_get(f);
	if (IS_ERR(map))
		return PTR_ERR(map);
	if (!(map_get_sys_perms(map, f) & FMODE_CAN_WRITE)) {
		err = -EPERM;
		goto err_put;
	}

	if ((attr->flags & BPF_F_LOCK) &&
	    !map_value_has_spin_lock(map)) {
		err = -EINVAL;
		goto err_put;
	}

	key = ___bpf_copy_key(ukey, map->key_size);
	if (IS_ERR(key)) {
		err = PTR_ERR(key);
		goto err_put;
	}

	value_size = bpf_map_value_size(map);

	err = -ENOMEM;
	value = kvmalloc(value_size, GFP_USER | __GFP_NOWARN);
	if (!value)
		goto free_key;

	err = -EFAULT;
	if (copy_from_bpfptr(value, uvalue, value_size) != 0)
		goto free_value;

	err = bpf_map_update_value(map, f, key, value, attr->flags);

free_value:
	kvfree(value);
free_key:
	kvfree(key);
err_put:
	fdput(f);
	return err;
}