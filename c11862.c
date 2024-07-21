static int efi_capsule_release(struct inode *inode, struct file *file)
{
	struct capsule_info *cap_info = file->private_data;

	kfree(cap_info->pages);
	kfree(cap_info->phys);
	kfree(file->private_data);
	file->private_data = NULL;
	return 0;
}