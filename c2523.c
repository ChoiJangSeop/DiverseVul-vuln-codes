int do_remount_sb(struct super_block *sb, int flags, void *data, int force)
{
	int retval;
	int remount_ro;

	if (sb->s_writers.frozen != SB_UNFROZEN)
		return -EBUSY;

#ifdef CONFIG_BLOCK
	if (!(flags & MS_RDONLY) && bdev_read_only(sb->s_bdev))
		return -EACCES;
#endif

	if (flags & MS_RDONLY)
		acct_auto_close(sb);
	shrink_dcache_sb(sb);
	sync_filesystem(sb);

	remount_ro = (flags & MS_RDONLY) && !(sb->s_flags & MS_RDONLY);

	/* If we are remounting RDONLY and current sb is read/write,
	   make sure there are no rw files opened */
	if (remount_ro) {
		if (force) {
			mark_files_ro(sb);
		} else {
			retval = sb_prepare_remount_readonly(sb);
			if (retval)
				return retval;
		}
	}

	if (sb->s_op->remount_fs) {
		retval = sb->s_op->remount_fs(sb, &flags, data);
		if (retval) {
			if (!force)
				goto cancel_readonly;
			/* If forced remount, go ahead despite any errors */
			WARN(1, "forced remount of a %s fs returned %i\n",
			     sb->s_type->name, retval);
		}
	}
	sb->s_flags = (sb->s_flags & ~MS_RMT_MASK) | (flags & MS_RMT_MASK);
	/* Needs to be ordered wrt mnt_is_readonly() */
	smp_wmb();
	sb->s_readonly_remount = 0;

	/*
	 * Some filesystems modify their metadata via some other path than the
	 * bdev buffer cache (eg. use a private mapping, or directories in
	 * pagecache, etc). Also file data modifications go via their own
	 * mappings. So If we try to mount readonly then copy the filesystem
	 * from bdev, we could get stale data, so invalidate it to give a best
	 * effort at coherency.
	 */
	if (remount_ro && sb->s_bdev)
		invalidate_bdev(sb->s_bdev);
	return 0;

cancel_readonly:
	sb->s_readonly_remount = 0;
	return retval;
}