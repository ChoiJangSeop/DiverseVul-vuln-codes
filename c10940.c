nsim_bpf_map_alloc(struct netdevsim *ns, struct bpf_offloaded_map *offmap)
{
	struct nsim_bpf_bound_map *nmap;
	int i, err;

	if (WARN_ON(offmap->map.map_type != BPF_MAP_TYPE_ARRAY &&
		    offmap->map.map_type != BPF_MAP_TYPE_HASH))
		return -EINVAL;
	if (offmap->map.max_entries > NSIM_BPF_MAX_KEYS)
		return -ENOMEM;
	if (offmap->map.map_flags)
		return -EINVAL;

	nmap = kzalloc(sizeof(*nmap), GFP_USER);
	if (!nmap)
		return -ENOMEM;

	offmap->dev_priv = nmap;
	nmap->ns = ns;
	nmap->map = offmap;
	mutex_init(&nmap->mutex);

	if (offmap->map.map_type == BPF_MAP_TYPE_ARRAY) {
		for (i = 0; i < ARRAY_SIZE(nmap->entry); i++) {
			u32 *key;

			err = nsim_map_alloc_elem(offmap, i);
			if (err)
				goto err_free;
			key = nmap->entry[i].key;
			*key = i;
		}
	}

	offmap->dev_ops = &nsim_bpf_map_ops;
	list_add_tail(&nmap->l, &ns->nsim_dev->bpf_bound_maps);

	return 0;

err_free:
	while (--i >= 0) {
		kfree(nmap->entry[i].key);
		kfree(nmap->entry[i].value);
	}
	kfree(nmap);
	return err;
}