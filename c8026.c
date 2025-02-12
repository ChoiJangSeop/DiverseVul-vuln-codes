int v9fs_refresh_inode_dotl(struct p9_fid *fid, struct inode *inode)
{
	loff_t i_size;
	struct p9_stat_dotl *st;
	struct v9fs_session_info *v9ses;

	v9ses = v9fs_inode2v9ses(inode);
	st = p9_client_getattr_dotl(fid, P9_STATS_ALL);
	if (IS_ERR(st))
		return PTR_ERR(st);
	/*
	 * Don't update inode if the file type is different
	 */
	if ((inode->i_mode & S_IFMT) != (st->st_mode & S_IFMT))
		goto out;

	spin_lock(&inode->i_lock);
	/*
	 * We don't want to refresh inode->i_size,
	 * because we may have cached data
	 */
	i_size = inode->i_size;
	v9fs_stat2inode_dotl(st, inode);
	if (v9ses->cache == CACHE_LOOSE || v9ses->cache == CACHE_FSCACHE)
		inode->i_size = i_size;
	spin_unlock(&inode->i_lock);
out:
	kfree(st);
	return 0;
}