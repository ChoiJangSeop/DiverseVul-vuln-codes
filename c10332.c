ssize_t fuse_listxattr(struct dentry *entry, char *list, size_t size)
{
	struct inode *inode = d_inode(entry);
	struct fuse_mount *fm = get_fuse_mount(inode);
	FUSE_ARGS(args);
	struct fuse_getxattr_in inarg;
	struct fuse_getxattr_out outarg;
	ssize_t ret;

	if (!fuse_allow_current_process(fm->fc))
		return -EACCES;

	if (fm->fc->no_listxattr)
		return -EOPNOTSUPP;

	memset(&inarg, 0, sizeof(inarg));
	inarg.size = size;
	args.opcode = FUSE_LISTXATTR;
	args.nodeid = get_node_id(inode);
	args.in_numargs = 1;
	args.in_args[0].size = sizeof(inarg);
	args.in_args[0].value = &inarg;
	/* This is really two different operations rolled into one */
	args.out_numargs = 1;
	if (size) {
		args.out_argvar = true;
		args.out_args[0].size = size;
		args.out_args[0].value = list;
	} else {
		args.out_args[0].size = sizeof(outarg);
		args.out_args[0].value = &outarg;
	}
	ret = fuse_simple_request(fm, &args);
	if (!ret && !size)
		ret = min_t(ssize_t, outarg.size, XATTR_LIST_MAX);
	if (ret > 0 && size)
		ret = fuse_verify_xattr_list(list, ret);
	if (ret == -ENOSYS) {
		fm->fc->no_listxattr = 1;
		ret = -EOPNOTSUPP;
	}
	return ret;
}