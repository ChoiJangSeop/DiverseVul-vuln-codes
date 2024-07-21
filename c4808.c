static int tstats_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, tstats_show, NULL);
}