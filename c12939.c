SYSCALL_DEFINE1(inotify_init1, int, flags)
{
	struct fsnotify_group *group;
	struct user_struct *user;
	int ret;

	/* Check the IN_* constants for consistency.  */
	BUILD_BUG_ON(IN_CLOEXEC != O_CLOEXEC);
	BUILD_BUG_ON(IN_NONBLOCK != O_NONBLOCK);

	if (flags & ~(IN_CLOEXEC | IN_NONBLOCK))
		return -EINVAL;

	user = get_current_user();
	if (unlikely(atomic_read(&user->inotify_devs) >=
			inotify_max_user_instances)) {
		ret = -EMFILE;
		goto out_free_uid;
	}

	/* fsnotify_obtain_group took a reference to group, we put this when we kill the file in the end */
	group = inotify_new_group(user, inotify_max_queued_events);
	if (IS_ERR(group)) {
		ret = PTR_ERR(group);
		goto out_free_uid;
	}

	atomic_inc(&user->inotify_devs);

	ret = anon_inode_getfd("inotify", &inotify_fops, group,
				  O_RDONLY | flags);
	if (ret >= 0)
		return ret;

	fsnotify_put_group(group);
	atomic_dec(&user->inotify_devs);
out_free_uid:
	free_uid(user);
	return ret;
}