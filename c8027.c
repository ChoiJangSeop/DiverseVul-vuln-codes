int v9fs_refresh_inode(struct p9_fid *fid, struct inode *inode)
{
	int umode;
	dev_t rdev;
	loff_t i_size;
	struct p9_wstat *st;
	struct v9fs_session_info *v9ses;

	v9ses = v9fs_inode2v9ses(inode);
	st = p9_client_stat(fid);
	if (IS_ERR(st))
		return PTR_ERR(st);
	/*
	 * Don't update inode if the file type is different
	 */
	umode = p9mode2unixmode(v9ses, st, &rdev);
	if ((inode->i_mode & S_IFMT) != (umode & S_IFMT))
		goto out;

	spin_lock(&inode->i_lock);
	/*
	 * We don't want to refresh inode->i_size,
	 * because we may have cached data
	 */
	i_size = inode->i_size;
	v9fs_stat2inode(st, inode, inode->i_sb);
	if (v9ses->cache == CACHE_LOOSE || v9ses->cache == CACHE_FSCACHE)
		inode->i_size = i_size;
	spin_unlock(&inode->i_lock);
out:
	p9stat_free(st);
	kfree(st);
	return 0;
}