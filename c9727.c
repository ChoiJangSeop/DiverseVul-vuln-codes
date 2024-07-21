static int hfi1_file_open(struct inode *inode, struct file *fp)
{
	struct hfi1_filedata *fd;
	struct hfi1_devdata *dd = container_of(inode->i_cdev,
					       struct hfi1_devdata,
					       user_cdev);

	if (!((dd->flags & HFI1_PRESENT) && dd->kregbase1))
		return -EINVAL;

	if (!atomic_inc_not_zero(&dd->user_refcount))
		return -ENXIO;

	/* The real work is performed later in assign_ctxt() */

	fd = kzalloc(sizeof(*fd), GFP_KERNEL);

	if (!fd || init_srcu_struct(&fd->pq_srcu))
		goto nomem;
	spin_lock_init(&fd->pq_rcu_lock);
	spin_lock_init(&fd->tid_lock);
	spin_lock_init(&fd->invalid_lock);
	fd->rec_cpu_num = -1; /* no cpu affinity by default */
	fd->mm = current->mm;
	mmgrab(fd->mm);
	fd->dd = dd;
	fp->private_data = fd;
	return 0;
nomem:
	kfree(fd);
	fp->private_data = NULL;
	if (atomic_dec_and_test(&dd->user_refcount))
		complete(&dd->user_comp);
	return -ENOMEM;
}