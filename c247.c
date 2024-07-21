void inotify_destroy(struct inotify_handle *ih)
{
	/*
	 * Destroy all of the watches for this handle. Unfortunately, not very
	 * pretty.  We cannot do a simple iteration over the list, because we
	 * do not know the inode until we iterate to the watch.  But we need to
	 * hold inode->inotify_mutex before ih->mutex.  The following works.
	 */
	while (1) {
		struct inotify_watch *watch;
		struct list_head *watches;
		struct inode *inode;

		mutex_lock(&ih->mutex);
		watches = &ih->watches;
		if (list_empty(watches)) {
			mutex_unlock(&ih->mutex);
			break;
		}
		watch = list_first_entry(watches, struct inotify_watch, h_list);
		get_inotify_watch(watch);
		mutex_unlock(&ih->mutex);

		inode = watch->inode;
		mutex_lock(&inode->inotify_mutex);
		mutex_lock(&ih->mutex);

		/* make sure we didn't race with another list removal */
		if (likely(idr_find(&ih->idr, watch->wd))) {
			remove_watch_no_event(watch, ih);
			put_inotify_watch(watch);
		}

		mutex_unlock(&ih->mutex);
		mutex_unlock(&inode->inotify_mutex);
		put_inotify_watch(watch);
	}

	/* free this handle: the put matching the get in inotify_init() */
	put_inotify_handle(ih);
}