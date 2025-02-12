static int shmem_symlink(struct inode *dir, struct dentry *dentry, const char *symname)
{
	int error;
	int len;
	struct inode *inode;
	struct page *page = NULL;
	char *kaddr;
	struct shmem_inode_info *info;

	len = strlen(symname) + 1;
	if (len > PAGE_CACHE_SIZE)
		return -ENAMETOOLONG;

	inode = shmem_get_inode(dir->i_sb, S_IFLNK|S_IRWXUGO, 0);
	if (!inode)
		return -ENOSPC;

	error = security_inode_init_security(inode, dir, NULL, NULL,
					     NULL);
	if (error) {
		if (error != -EOPNOTSUPP) {
			iput(inode);
			return error;
		}
		error = 0;
	}

	info = SHMEM_I(inode);
	inode->i_size = len-1;
	if (len <= (char *)inode - (char *)info) {
		/* do it inline */
		memcpy(info, symname, len);
		inode->i_op = &shmem_symlink_inline_operations;
	} else {
		error = shmem_getpage(inode, 0, &page, SGP_WRITE, NULL);
		if (error) {
			iput(inode);
			return error;
		}
		unlock_page(page);
		inode->i_op = &shmem_symlink_inode_operations;
		kaddr = kmap_atomic(page, KM_USER0);
		memcpy(kaddr, symname, len);
		kunmap_atomic(kaddr, KM_USER0);
		set_page_dirty(page);
		page_cache_release(page);
	}
	if (dir->i_mode & S_ISGID)
		inode->i_gid = dir->i_gid;
	dir->i_size += BOGO_DIRENT_SIZE;
	dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	d_instantiate(dentry, inode);
	dget(dentry);
	return 0;
}