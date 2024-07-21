static int efi_capsule_flush(struct file *file, fl_owner_t id)
{
	int ret = 0;
	struct capsule_info *cap_info = file->private_data;

	if (cap_info->index > 0) {
		pr_err("capsule upload not complete\n");
		efi_free_all_buff_pages(cap_info);
		ret = -ECANCELED;
	}

	return ret;
}