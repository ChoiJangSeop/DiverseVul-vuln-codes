int fuse_set_acl(struct inode *inode, struct posix_acl *acl, int type)
{
	struct fuse_conn *fc = get_fuse_conn(inode);
	const char *name;
	int ret;

	if (!fc->posix_acl || fc->no_setxattr)
		return -EOPNOTSUPP;

	if (type == ACL_TYPE_ACCESS)
		name = XATTR_NAME_POSIX_ACL_ACCESS;
	else if (type == ACL_TYPE_DEFAULT)
		name = XATTR_NAME_POSIX_ACL_DEFAULT;
	else
		return -EINVAL;

	if (acl) {
		/*
		 * Fuse userspace is responsible for updating access
		 * permissions in the inode, if needed. fuse_setxattr
		 * invalidates the inode attributes, which will force
		 * them to be refreshed the next time they are used,
		 * and it also updates i_ctime.
		 */
		size_t size = posix_acl_xattr_size(acl->a_count);
		void *value;

		if (size > PAGE_SIZE)
			return -E2BIG;

		value = kmalloc(size, GFP_KERNEL);
		if (!value)
			return -ENOMEM;

		ret = posix_acl_to_xattr(fc->user_ns, acl, value, size);
		if (ret < 0) {
			kfree(value);
			return ret;
		}

		ret = fuse_setxattr(inode, name, value, size, 0);
		kfree(value);
	} else {
		ret = fuse_removexattr(inode, name);
	}
	forget_all_cached_acls(inode);
	fuse_invalidate_attr(inode);

	return ret;
}