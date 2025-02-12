static int ext4_xattr_set_entry(struct ext4_xattr_info *i,
				struct ext4_xattr_search *s,
				handle_t *handle, struct inode *inode,
				bool is_block)
{
	struct ext4_xattr_entry *last;
	struct ext4_xattr_entry *here = s->here;
	size_t min_offs = s->end - s->base, name_len = strlen(i->name);
	int in_inode = i->in_inode;
	struct inode *old_ea_inode = NULL;
	struct inode *new_ea_inode = NULL;
	size_t old_size, new_size;
	int ret;

	/* Space used by old and new values. */
	old_size = (!s->not_found && !here->e_value_inum) ?
			EXT4_XATTR_SIZE(le32_to_cpu(here->e_value_size)) : 0;
	new_size = (i->value && !in_inode) ? EXT4_XATTR_SIZE(i->value_len) : 0;

	/*
	 * Optimization for the simple case when old and new values have the
	 * same padded sizes. Not applicable if external inodes are involved.
	 */
	if (new_size && new_size == old_size) {
		size_t offs = le16_to_cpu(here->e_value_offs);
		void *val = s->base + offs;

		here->e_value_size = cpu_to_le32(i->value_len);
		if (i->value == EXT4_ZERO_XATTR_VALUE) {
			memset(val, 0, new_size);
		} else {
			memcpy(val, i->value, i->value_len);
			/* Clear padding bytes. */
			memset(val + i->value_len, 0, new_size - i->value_len);
		}
		goto update_hash;
	}

	/* Compute min_offs and last. */
	last = s->first;
	for (; !IS_LAST_ENTRY(last); last = EXT4_XATTR_NEXT(last)) {
		if (!last->e_value_inum && last->e_value_size) {
			size_t offs = le16_to_cpu(last->e_value_offs);
			if (offs < min_offs)
				min_offs = offs;
		}
	}

	/* Check whether we have enough space. */
	if (i->value) {
		size_t free;

		free = min_offs - ((void *)last - s->base) - sizeof(__u32);
		if (!s->not_found)
			free += EXT4_XATTR_LEN(name_len) + old_size;

		if (free < EXT4_XATTR_LEN(name_len) + new_size) {
			ret = -ENOSPC;
			goto out;
		}

		/*
		 * If storing the value in an external inode is an option,
		 * reserve space for xattr entries/names in the external
		 * attribute block so that a long value does not occupy the
		 * whole space and prevent futher entries being added.
		 */
		if (ext4_has_feature_ea_inode(inode->i_sb) &&
		    new_size && is_block &&
		    (min_offs + old_size - new_size) <
					EXT4_XATTR_BLOCK_RESERVE(inode)) {
			ret = -ENOSPC;
			goto out;
		}
	}

	/*
	 * Getting access to old and new ea inodes is subject to failures.
	 * Finish that work before doing any modifications to the xattr data.
	 */
	if (!s->not_found && here->e_value_inum) {
		ret = ext4_xattr_inode_iget(inode,
					    le32_to_cpu(here->e_value_inum),
					    le32_to_cpu(here->e_hash),
					    &old_ea_inode);
		if (ret) {
			old_ea_inode = NULL;
			goto out;
		}
	}
	if (i->value && in_inode) {
		WARN_ON_ONCE(!i->value_len);

		ret = ext4_xattr_inode_alloc_quota(inode, i->value_len);
		if (ret)
			goto out;

		ret = ext4_xattr_inode_lookup_create(handle, inode, i->value,
						     i->value_len,
						     &new_ea_inode);
		if (ret) {
			new_ea_inode = NULL;
			ext4_xattr_inode_free_quota(inode, NULL, i->value_len);
			goto out;
		}
	}

	if (old_ea_inode) {
		/* We are ready to release ref count on the old_ea_inode. */
		ret = ext4_xattr_inode_dec_ref(handle, old_ea_inode);
		if (ret) {
			/* Release newly required ref count on new_ea_inode. */
			if (new_ea_inode) {
				int err;

				err = ext4_xattr_inode_dec_ref(handle,
							       new_ea_inode);
				if (err)
					ext4_warning_inode(new_ea_inode,
						  "dec ref new_ea_inode err=%d",
						  err);
				ext4_xattr_inode_free_quota(inode, new_ea_inode,
							    i->value_len);
			}
			goto out;
		}

		ext4_xattr_inode_free_quota(inode, old_ea_inode,
					    le32_to_cpu(here->e_value_size));
	}

	/* No failures allowed past this point. */

	if (!s->not_found && here->e_value_offs) {
		/* Remove the old value. */
		void *first_val = s->base + min_offs;
		size_t offs = le16_to_cpu(here->e_value_offs);
		void *val = s->base + offs;

		memmove(first_val + old_size, first_val, val - first_val);
		memset(first_val, 0, old_size);
		min_offs += old_size;

		/* Adjust all value offsets. */
		last = s->first;
		while (!IS_LAST_ENTRY(last)) {
			size_t o = le16_to_cpu(last->e_value_offs);

			if (!last->e_value_inum &&
			    last->e_value_size && o < offs)
				last->e_value_offs = cpu_to_le16(o + old_size);
			last = EXT4_XATTR_NEXT(last);
		}
	}

	if (!i->value) {
		/* Remove old name. */
		size_t size = EXT4_XATTR_LEN(name_len);

		last = ENTRY((void *)last - size);
		memmove(here, (void *)here + size,
			(void *)last - (void *)here + sizeof(__u32));
		memset(last, 0, size);
	} else if (s->not_found) {
		/* Insert new name. */
		size_t size = EXT4_XATTR_LEN(name_len);
		size_t rest = (void *)last - (void *)here + sizeof(__u32);

		memmove((void *)here + size, here, rest);
		memset(here, 0, size);
		here->e_name_index = i->name_index;
		here->e_name_len = name_len;
		memcpy(here->e_name, i->name, name_len);
	} else {
		/* This is an update, reset value info. */
		here->e_value_inum = 0;
		here->e_value_offs = 0;
		here->e_value_size = 0;
	}

	if (i->value) {
		/* Insert new value. */
		if (in_inode) {
			here->e_value_inum = cpu_to_le32(new_ea_inode->i_ino);
		} else if (i->value_len) {
			void *val = s->base + min_offs - new_size;

			here->e_value_offs = cpu_to_le16(min_offs - new_size);
			if (i->value == EXT4_ZERO_XATTR_VALUE) {
				memset(val, 0, new_size);
			} else {
				memcpy(val, i->value, i->value_len);
				/* Clear padding bytes. */
				memset(val + i->value_len, 0,
				       new_size - i->value_len);
			}
		}
		here->e_value_size = cpu_to_le32(i->value_len);
	}

update_hash:
	if (i->value) {
		__le32 hash = 0;

		/* Entry hash calculation. */
		if (in_inode) {
			__le32 crc32c_hash;

			/*
			 * Feed crc32c hash instead of the raw value for entry
			 * hash calculation. This is to avoid walking
			 * potentially long value buffer again.
			 */
			crc32c_hash = cpu_to_le32(
				       ext4_xattr_inode_get_hash(new_ea_inode));
			hash = ext4_xattr_hash_entry(here->e_name,
						     here->e_name_len,
						     &crc32c_hash, 1);
		} else if (is_block) {
			__le32 *value = s->base + le16_to_cpu(
							here->e_value_offs);

			hash = ext4_xattr_hash_entry(here->e_name,
						     here->e_name_len, value,
						     new_size >> 2);
		}
		here->e_hash = hash;
	}

	if (is_block)
		ext4_xattr_rehash((struct ext4_xattr_header *)s->base);

	ret = 0;
out:
	iput(old_ea_inode);
	iput(new_ea_inode);
	return ret;
}