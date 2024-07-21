static int psi_fop_release(struct inode *inode, struct file *file)
{
	struct seq_file *seq = file->private_data;

	psi_trigger_replace(&seq->private, NULL);
	return single_release(inode, file);
}