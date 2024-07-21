int inotify_rm_wd(struct inotify_handle *ih, u32 wd)
{
	struct inotify_watch *watch;
	struct inode *inode;

	mutex_lock(&ih->mutex);
	watch = idr_find(&ih->idr, wd);
	if (unlikely(!watch)) {
		mutex_unlock(&ih->mutex);
		return -EINVAL;
	}
	get_inotify_watch(watch);
	inode = watch->inode;
	mutex_unlock(&ih->mutex);

	mutex_lock(&inode->inotify_mutex);
	mutex_lock(&ih->mutex);

	/* make sure that we did not race */
	if (likely(idr_find(&ih->idr, wd) == watch))
		inotify_remove_watch_locked(ih, watch);

	mutex_unlock(&ih->mutex);
	mutex_unlock(&inode->inotify_mutex);
	put_inotify_watch(watch);

	return 0;
}