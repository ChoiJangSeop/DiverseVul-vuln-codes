struct posix_acl *ovl_get_acl(struct inode *inode, int type)
{
	struct inode *realinode = ovl_inode_real(inode);

	if (!realinode)
		return ERR_PTR(-ENOENT);

	if (!IS_POSIXACL(realinode))
		return NULL;

	if (!realinode->i_op->get_acl)
		return NULL;

	return realinode->i_op->get_acl(realinode, type);
}