SYSCALL_DEFINE2(fchmod, unsigned int, fd, mode_t, mode)
{
	struct inode * inode;
	struct dentry * dentry;
	struct file * file;
	int err = -EBADF;
	struct iattr newattrs;

	file = fget(fd);
	if (!file)
		goto out;

	dentry = file->f_path.dentry;
	inode = dentry->d_inode;

	audit_inode(NULL, dentry);

	err = mnt_want_write_file(file);
	if (err)
		goto out_putf;
	mutex_lock(&inode->i_mutex);
	err = security_path_chmod(dentry, file->f_vfsmnt, mode);
	if (err)
		goto out_unlock;
	if (mode == (mode_t) -1)
		mode = inode->i_mode;
	newattrs.ia_mode = (mode & S_IALLUGO) | (inode->i_mode & ~S_IALLUGO);
	newattrs.ia_valid = ATTR_MODE | ATTR_CTIME;
	err = notify_change(dentry, &newattrs);
out_unlock:
	mutex_unlock(&inode->i_mutex);
	mnt_drop_write(file->f_path.mnt);
out_putf:
	fput(file);
out:
	return err;
}