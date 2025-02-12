int ext4_ext_insert_extent(handle_t *handle, struct inode *inode,
				struct ext4_ext_path *path,
				struct ext4_extent *newext, int flag)
{
	struct ext4_extent_header *eh;
	struct ext4_extent *ex, *fex;
	struct ext4_extent *nearex; /* nearest extent */
	struct ext4_ext_path *npath = NULL;
	int depth, len, err;
	ext4_lblk_t next;
	unsigned uninitialized = 0;

	BUG_ON(ext4_ext_get_actual_len(newext) == 0);
	depth = ext_depth(inode);
	ex = path[depth].p_ext;
	BUG_ON(path[depth].p_hdr == NULL);

	/* try to insert block into found extent and return */
	if (ex && (flag != EXT4_GET_BLOCKS_PRE_IO)
		&& ext4_can_extents_be_merged(inode, ex, newext)) {
		ext_debug("append [%d]%d block to %d:[%d]%d (from %llu)\n",
				ext4_ext_is_uninitialized(newext),
				ext4_ext_get_actual_len(newext),
				le32_to_cpu(ex->ee_block),
				ext4_ext_is_uninitialized(ex),
				ext4_ext_get_actual_len(ex), ext_pblock(ex));
		err = ext4_ext_get_access(handle, inode, path + depth);
		if (err)
			return err;

		/*
		 * ext4_can_extents_be_merged should have checked that either
		 * both extents are uninitialized, or both aren't. Thus we
		 * need to check only one of them here.
		 */
		if (ext4_ext_is_uninitialized(ex))
			uninitialized = 1;
		ex->ee_len = cpu_to_le16(ext4_ext_get_actual_len(ex)
					+ ext4_ext_get_actual_len(newext));
		if (uninitialized)
			ext4_ext_mark_uninitialized(ex);
		eh = path[depth].p_hdr;
		nearex = ex;
		goto merge;
	}

repeat:
	depth = ext_depth(inode);
	eh = path[depth].p_hdr;
	if (le16_to_cpu(eh->eh_entries) < le16_to_cpu(eh->eh_max))
		goto has_space;

	/* probably next leaf has space for us? */
	fex = EXT_LAST_EXTENT(eh);
	next = ext4_ext_next_leaf_block(inode, path);
	if (le32_to_cpu(newext->ee_block) > le32_to_cpu(fex->ee_block)
	    && next != EXT_MAX_BLOCK) {
		ext_debug("next leaf block - %d\n", next);
		BUG_ON(npath != NULL);
		npath = ext4_ext_find_extent(inode, next, NULL);
		if (IS_ERR(npath))
			return PTR_ERR(npath);
		BUG_ON(npath->p_depth != path->p_depth);
		eh = npath[depth].p_hdr;
		if (le16_to_cpu(eh->eh_entries) < le16_to_cpu(eh->eh_max)) {
			ext_debug("next leaf isnt full(%d)\n",
				  le16_to_cpu(eh->eh_entries));
			path = npath;
			goto repeat;
		}
		ext_debug("next leaf has no free space(%d,%d)\n",
			  le16_to_cpu(eh->eh_entries), le16_to_cpu(eh->eh_max));
	}

	/*
	 * There is no free space in the found leaf.
	 * We're gonna add a new leaf in the tree.
	 */
	err = ext4_ext_create_new_leaf(handle, inode, path, newext);
	if (err)
		goto cleanup;
	depth = ext_depth(inode);
	eh = path[depth].p_hdr;

has_space:
	nearex = path[depth].p_ext;

	err = ext4_ext_get_access(handle, inode, path + depth);
	if (err)
		goto cleanup;

	if (!nearex) {
		/* there is no extent in this leaf, create first one */
		ext_debug("first extent in the leaf: %d:%llu:[%d]%d\n",
				le32_to_cpu(newext->ee_block),
				ext_pblock(newext),
				ext4_ext_is_uninitialized(newext),
				ext4_ext_get_actual_len(newext));
		path[depth].p_ext = EXT_FIRST_EXTENT(eh);
	} else if (le32_to_cpu(newext->ee_block)
			   > le32_to_cpu(nearex->ee_block)) {
/*		BUG_ON(newext->ee_block == nearex->ee_block); */
		if (nearex != EXT_LAST_EXTENT(eh)) {
			len = EXT_MAX_EXTENT(eh) - nearex;
			len = (len - 1) * sizeof(struct ext4_extent);
			len = len < 0 ? 0 : len;
			ext_debug("insert %d:%llu:[%d]%d after: nearest 0x%p, "
					"move %d from 0x%p to 0x%p\n",
					le32_to_cpu(newext->ee_block),
					ext_pblock(newext),
					ext4_ext_is_uninitialized(newext),
					ext4_ext_get_actual_len(newext),
					nearex, len, nearex + 1, nearex + 2);
			memmove(nearex + 2, nearex + 1, len);
		}
		path[depth].p_ext = nearex + 1;
	} else {
		BUG_ON(newext->ee_block == nearex->ee_block);
		len = (EXT_MAX_EXTENT(eh) - nearex) * sizeof(struct ext4_extent);
		len = len < 0 ? 0 : len;
		ext_debug("insert %d:%llu:[%d]%d before: nearest 0x%p, "
				"move %d from 0x%p to 0x%p\n",
				le32_to_cpu(newext->ee_block),
				ext_pblock(newext),
				ext4_ext_is_uninitialized(newext),
				ext4_ext_get_actual_len(newext),
				nearex, len, nearex + 1, nearex + 2);
		memmove(nearex + 1, nearex, len);
		path[depth].p_ext = nearex;
	}

	le16_add_cpu(&eh->eh_entries, 1);
	nearex = path[depth].p_ext;
	nearex->ee_block = newext->ee_block;
	ext4_ext_store_pblock(nearex, ext_pblock(newext));
	nearex->ee_len = newext->ee_len;

merge:
	/* try to merge extents to the right */
	if (flag != EXT4_GET_BLOCKS_PRE_IO)
		ext4_ext_try_to_merge(inode, path, nearex);

	/* try to merge extents to the left */

	/* time to correct all indexes above */
	err = ext4_ext_correct_indexes(handle, inode, path);
	if (err)
		goto cleanup;

	err = ext4_ext_dirty(handle, inode, path + depth);

cleanup:
	if (npath) {
		ext4_ext_drop_refs(npath);
		kfree(npath);
	}
	ext4_ext_invalidate_cache(inode);
	return err;
}