static bool has_locked_children(struct mount *mnt, struct dentry *dentry)
{
	struct mount *child;
	list_for_each_entry(child, &mnt->mnt_mounts, mnt_child) {
		if (!is_subdir(child->mnt_mountpoint, dentry))
			continue;

		if (child->mnt.mnt_flags & MNT_LOCKED)
			return true;
	}
	return false;
}