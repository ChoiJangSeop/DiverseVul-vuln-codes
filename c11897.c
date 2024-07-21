proc_file_lseek(struct file *file, loff_t offset, int orig)
{
    lock_kernel();

    switch (orig) {
    case 0:
	if (offset < 0)
	    goto out;
	file->f_pos = offset;
	unlock_kernel();
	return(file->f_pos);
    case 1:
	if (offset + file->f_pos < 0)
	    goto out;
	file->f_pos += offset;
	unlock_kernel();
	return(file->f_pos);
    case 2:
	goto out;
    default:
	goto out;
    }

out:
    unlock_kernel();
    return -EINVAL;
}