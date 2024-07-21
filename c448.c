static int gfs2_adjust_quota(struct gfs2_inode *ip, loff_t loc,
			     s64 change, struct gfs2_quota_data *qd,
			     struct fs_disk_quota *fdq)
{
	struct inode *inode = &ip->i_inode;
	struct address_space *mapping = inode->i_mapping;
	unsigned long index = loc >> PAGE_CACHE_SHIFT;
	unsigned offset = loc & (PAGE_CACHE_SIZE - 1);
	unsigned blocksize, iblock, pos;
	struct buffer_head *bh, *dibh;
	struct page *page;
	void *kaddr;
	struct gfs2_quota *qp;
	s64 value;
	int err = -EIO;
	u64 size;

	if (gfs2_is_stuffed(ip))
		gfs2_unstuff_dinode(ip, NULL);
	
	page = grab_cache_page(mapping, index);
	if (!page)
		return -ENOMEM;

	blocksize = inode->i_sb->s_blocksize;
	iblock = index << (PAGE_CACHE_SHIFT - inode->i_sb->s_blocksize_bits);

	if (!page_has_buffers(page))
		create_empty_buffers(page, blocksize, 0);

	bh = page_buffers(page);
	pos = blocksize;
	while (offset >= pos) {
		bh = bh->b_this_page;
		iblock++;
		pos += blocksize;
	}

	if (!buffer_mapped(bh)) {
		gfs2_block_map(inode, iblock, bh, 1);
		if (!buffer_mapped(bh))
			goto unlock;
	}

	if (PageUptodate(page))
		set_buffer_uptodate(bh);

	if (!buffer_uptodate(bh)) {
		ll_rw_block(READ_META, 1, &bh);
		wait_on_buffer(bh);
		if (!buffer_uptodate(bh))
			goto unlock;
	}

	gfs2_trans_add_bh(ip->i_gl, bh, 0);

	kaddr = kmap_atomic(page, KM_USER0);
	qp = kaddr + offset;
	value = (s64)be64_to_cpu(qp->qu_value) + change;
	qp->qu_value = cpu_to_be64(value);
	qd->qd_qb.qb_value = qp->qu_value;
	if (fdq) {
		if (fdq->d_fieldmask & FS_DQ_BSOFT) {
			qp->qu_warn = cpu_to_be64(fdq->d_blk_softlimit);
			qd->qd_qb.qb_warn = qp->qu_warn;
		}
		if (fdq->d_fieldmask & FS_DQ_BHARD) {
			qp->qu_limit = cpu_to_be64(fdq->d_blk_hardlimit);
			qd->qd_qb.qb_limit = qp->qu_limit;
		}
	}
	flush_dcache_page(page);
	kunmap_atomic(kaddr, KM_USER0);

	err = gfs2_meta_inode_buffer(ip, &dibh);
	if (err)
		goto unlock;

	size = loc + sizeof(struct gfs2_quota);
	if (size > inode->i_size) {
		ip->i_disksize = size;
		i_size_write(inode, size);
	}
	inode->i_mtime = inode->i_atime = CURRENT_TIME;
	gfs2_trans_add_bh(ip->i_gl, dibh, 1);
	gfs2_dinode_out(ip, dibh->b_data);
	brelse(dibh);
	mark_inode_dirty(inode);

unlock:
	unlock_page(page);
	page_cache_release(page);
	return err;
}