static void bpf_map_mmap_close(struct vm_area_struct *vma)
{
	struct bpf_map *map = vma->vm_file->private_data;

	if (vma->vm_flags & VM_MAYWRITE) {
		mutex_lock(&map->freeze_mutex);
		map->writecnt--;
		mutex_unlock(&map->freeze_mutex);
	}
}