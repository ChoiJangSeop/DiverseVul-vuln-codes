static int bpf_map_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct bpf_map *map = filp->private_data;
	int err;

	if (!map->ops->map_mmap || map_value_has_spin_lock(map) ||
	    map_value_has_timer(map))
		return -ENOTSUPP;

	if (!(vma->vm_flags & VM_SHARED))
		return -EINVAL;

	mutex_lock(&map->freeze_mutex);

	if (vma->vm_flags & VM_WRITE) {
		if (map->frozen) {
			err = -EPERM;
			goto out;
		}
		/* map is meant to be read-only, so do not allow mapping as
		 * writable, because it's possible to leak a writable page
		 * reference and allows user-space to still modify it after
		 * freezing, while verifier will assume contents do not change
		 */
		if (map->map_flags & BPF_F_RDONLY_PROG) {
			err = -EACCES;
			goto out;
		}
	}

	/* set default open/close callbacks */
	vma->vm_ops = &bpf_map_default_vmops;
	vma->vm_private_data = map;
	vma->vm_flags &= ~VM_MAYEXEC;
	if (!(vma->vm_flags & VM_WRITE))
		/* disallow re-mapping with PROT_WRITE */
		vma->vm_flags &= ~VM_MAYWRITE;

	err = map->ops->map_mmap(map, vma);
	if (err)
		goto out;

	if (vma->vm_flags & VM_MAYWRITE)
		map->writecnt++;
out:
	mutex_unlock(&map->freeze_mutex);
	return err;
}