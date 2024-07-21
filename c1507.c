SYSCALL_DEFINE3(fchmodat, int, dfd, const char __user *, filename, mode_t, mode)
{
	struct path path;
	struct inode *inode;
	int error;
	struct iattr newattrs;

	error = user_path_at(dfd, filename, LOOKUP_FOLLOW, &path);
	if (error)
		goto out;
	inode = path.dentry->d_inode;

	error = mnt_want_write(path.mnt);
	if (error)
		goto dput_and_out;
	mutex_lock(&inode->i_mutex);
	error = security_path_chmod(path.dentry, path.mnt, mode);
	if (error)
		goto out_unlock;
	if (mode == (mode_t) -1)
		mode = inode->i_mode;
	newattrs.ia_mode = (mode & S_IALLUGO) | (inode->i_mode & ~S_IALLUGO);
	newattrs.ia_valid = ATTR_MODE | ATTR_CTIME;
	error = notify_change(path.dentry, &newattrs);
out_unlock:
	mutex_unlock(&inode->i_mutex);
	mnt_drop_write(path.mnt);
dput_and_out:
	path_put(&path);
out:
	return error;
}