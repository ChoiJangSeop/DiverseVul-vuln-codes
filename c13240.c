static int ext4_split_unwritten_extents(handle_t *handle,
					struct inode *inode,
					struct ext4_map_blocks *map,
					struct ext4_ext_path *path,
					int flags)
{
	struct ext4_extent *ex, newex, orig_ex;
	struct ext4_extent *ex1 = NULL;
	struct ext4_extent *ex2 = NULL;
	struct ext4_extent *ex3 = NULL;
	ext4_lblk_t ee_block, eof_block;
	unsigned int allocated, ee_len, depth;
	ext4_fsblk_t newblock;
	int err = 0;
	int may_zeroout;

	ext_debug("ext4_split_unwritten_extents: inode %lu, logical"
		"block %llu, max_blocks %u\n", inode->i_ino,
		(unsigned long long)map->m_lblk, map->m_len);

	eof_block = (inode->i_size + inode->i_sb->s_blocksize - 1) >>
		inode->i_sb->s_blocksize_bits;
	if (eof_block < map->m_lblk + map->m_len)
		eof_block = map->m_lblk + map->m_len;

	depth = ext_depth(inode);
	ex = path[depth].p_ext;
	ee_block = le32_to_cpu(ex->ee_block);
	ee_len = ext4_ext_get_actual_len(ex);
	allocated = ee_len - (map->m_lblk - ee_block);
	newblock = map->m_lblk - ee_block + ext4_ext_pblock(ex);

	ex2 = ex;
	orig_ex.ee_block = ex->ee_block;
	orig_ex.ee_len   = cpu_to_le16(ee_len);
	ext4_ext_store_pblock(&orig_ex, ext4_ext_pblock(ex));

	/*
	 * It is safe to convert extent to initialized via explicit
	 * zeroout only if extent is fully insde i_size or new_size.
	 */
	may_zeroout = ee_block + ee_len <= eof_block;

	/*
 	 * If the uninitialized extent begins at the same logical
 	 * block where the write begins, and the write completely
 	 * covers the extent, then we don't need to split it.
 	 */
	if ((map->m_lblk == ee_block) && (allocated <= map->m_len))
		return allocated;

	err = ext4_ext_get_access(handle, inode, path + depth);
	if (err)
		goto out;
	/* ex1: ee_block to map->m_lblk - 1 : uninitialized */
	if (map->m_lblk > ee_block) {
		ex1 = ex;
		ex1->ee_len = cpu_to_le16(map->m_lblk - ee_block);
		ext4_ext_mark_uninitialized(ex1);
		ex2 = &newex;
	}
	/*
	 * for sanity, update the length of the ex2 extent before
	 * we insert ex3, if ex1 is NULL. This is to avoid temporary
	 * overlap of blocks.
	 */
	if (!ex1 && allocated > map->m_len)
		ex2->ee_len = cpu_to_le16(map->m_len);
	/* ex3: to ee_block + ee_len : uninitialised */
	if (allocated > map->m_len) {
		unsigned int newdepth;
		ex3 = &newex;
		ex3->ee_block = cpu_to_le32(map->m_lblk + map->m_len);
		ext4_ext_store_pblock(ex3, newblock + map->m_len);
		ex3->ee_len = cpu_to_le16(allocated - map->m_len);
		ext4_ext_mark_uninitialized(ex3);
		err = ext4_ext_insert_extent(handle, inode, path, ex3, flags);
		if (err == -ENOSPC && may_zeroout) {
			err =  ext4_ext_zeroout(inode, &orig_ex);
			if (err)
				goto fix_extent_len;
			/* update the extent length and mark as initialized */
			ex->ee_block = orig_ex.ee_block;
			ex->ee_len   = orig_ex.ee_len;
			ext4_ext_store_pblock(ex, ext4_ext_pblock(&orig_ex));
			ext4_ext_dirty(handle, inode, path + depth);
			/* zeroed the full extent */
			/* blocks available from map->m_lblk */
			return allocated;

		} else if (err)
			goto fix_extent_len;
		/*
		 * The depth, and hence eh & ex might change
		 * as part of the insert above.
		 */
		newdepth = ext_depth(inode);
		/*
		 * update the extent length after successful insert of the
		 * split extent
		 */
		ee_len -= ext4_ext_get_actual_len(ex3);
		orig_ex.ee_len = cpu_to_le16(ee_len);
		may_zeroout = ee_block + ee_len <= eof_block;

		depth = newdepth;
		ext4_ext_drop_refs(path);
		path = ext4_ext_find_extent(inode, map->m_lblk, path);
		if (IS_ERR(path)) {
			err = PTR_ERR(path);
			goto out;
		}
		ex = path[depth].p_ext;
		if (ex2 != &newex)
			ex2 = ex;

		err = ext4_ext_get_access(handle, inode, path + depth);
		if (err)
			goto out;

		allocated = map->m_len;
	}
	/*
	 * If there was a change of depth as part of the
	 * insertion of ex3 above, we need to update the length
	 * of the ex1 extent again here
	 */
	if (ex1 && ex1 != ex) {
		ex1 = ex;
		ex1->ee_len = cpu_to_le16(map->m_lblk - ee_block);
		ext4_ext_mark_uninitialized(ex1);
		ex2 = &newex;
	}
	/*
	 * ex2: map->m_lblk to map->m_lblk + map->m_len-1 : to be written
	 * using direct I/O, uninitialised still.
	 */
	ex2->ee_block = cpu_to_le32(map->m_lblk);
	ext4_ext_store_pblock(ex2, newblock);
	ex2->ee_len = cpu_to_le16(allocated);
	ext4_ext_mark_uninitialized(ex2);
	if (ex2 != ex)
		goto insert;
	/* Mark modified extent as dirty */
	err = ext4_ext_dirty(handle, inode, path + depth);
	ext_debug("out here\n");
	goto out;
insert:
	err = ext4_ext_insert_extent(handle, inode, path, &newex, flags);
	if (err == -ENOSPC && may_zeroout) {
		err =  ext4_ext_zeroout(inode, &orig_ex);
		if (err)
			goto fix_extent_len;
		/* update the extent length and mark as initialized */
		ex->ee_block = orig_ex.ee_block;
		ex->ee_len   = orig_ex.ee_len;
		ext4_ext_store_pblock(ex, ext4_ext_pblock(&orig_ex));
		ext4_ext_dirty(handle, inode, path + depth);
		/* zero out the first half */
		return allocated;
	} else if (err)
		goto fix_extent_len;
out:
	ext4_ext_show_leaf(inode, path);
	return err ? err : allocated;

fix_extent_len:
	ex->ee_block = orig_ex.ee_block;
	ex->ee_len   = orig_ex.ee_len;
	ext4_ext_store_pblock(ex, ext4_ext_pblock(&orig_ex));
	ext4_ext_mark_uninitialized(ex);
	ext4_ext_dirty(handle, inode, path + depth);
	return err;
}