static int fallocate_chunk(struct inode *inode, loff_t offset, loff_t len,
			   int mode)
{
	struct gfs2_inode *ip = GFS2_I(inode);
	struct buffer_head *dibh;
	int error;
	u64 start = offset >> PAGE_CACHE_SHIFT;
	unsigned int start_offset = offset & ~PAGE_CACHE_MASK;
	u64 end = (offset + len - 1) >> PAGE_CACHE_SHIFT;
	pgoff_t curr;
	struct page *page;
	unsigned int end_offset = (offset + len) & ~PAGE_CACHE_MASK;
	unsigned int from, to;

	if (!end_offset)
		end_offset = PAGE_CACHE_SIZE;

	error = gfs2_meta_inode_buffer(ip, &dibh);
	if (unlikely(error))
		goto out;

	gfs2_trans_add_bh(ip->i_gl, dibh, 1);

	if (gfs2_is_stuffed(ip)) {
		error = gfs2_unstuff_dinode(ip, NULL);
		if (unlikely(error))
			goto out;
	}

	curr = start;
	offset = start << PAGE_CACHE_SHIFT;
	from = start_offset;
	to = PAGE_CACHE_SIZE;
	while (curr <= end) {
		page = grab_cache_page_write_begin(inode->i_mapping, curr,
						   AOP_FLAG_NOFS);
		if (unlikely(!page)) {
			error = -ENOMEM;
			goto out;
		}

		if (curr == end)
			to = end_offset;
		error = write_empty_blocks(page, from, to, mode);
		if (!error && offset + to > inode->i_size &&
		    !(mode & FALLOC_FL_KEEP_SIZE)) {
			i_size_write(inode, offset + to);
		}
		unlock_page(page);
		page_cache_release(page);
		if (error)
			goto out;
		curr++;
		offset += PAGE_CACHE_SIZE;
		from = 0;
	}

	mark_inode_dirty(inode);

	brelse(dibh);

out:
	return error;
}