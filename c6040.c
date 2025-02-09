int ocfs2_setattr(struct dentry *dentry, struct iattr *attr)
{
	int status = 0, size_change;
	int inode_locked = 0;
	struct inode *inode = d_inode(dentry);
	struct super_block *sb = inode->i_sb;
	struct ocfs2_super *osb = OCFS2_SB(sb);
	struct buffer_head *bh = NULL;
	handle_t *handle = NULL;
	struct dquot *transfer_to[MAXQUOTAS] = { };
	int qtype;
	int had_lock;
	struct ocfs2_lock_holder oh;

	trace_ocfs2_setattr(inode, dentry,
			    (unsigned long long)OCFS2_I(inode)->ip_blkno,
			    dentry->d_name.len, dentry->d_name.name,
			    attr->ia_valid, attr->ia_mode,
			    from_kuid(&init_user_ns, attr->ia_uid),
			    from_kgid(&init_user_ns, attr->ia_gid));

	/* ensuring we don't even attempt to truncate a symlink */
	if (S_ISLNK(inode->i_mode))
		attr->ia_valid &= ~ATTR_SIZE;

#define OCFS2_VALID_ATTRS (ATTR_ATIME | ATTR_MTIME | ATTR_CTIME | ATTR_SIZE \
			   | ATTR_GID | ATTR_UID | ATTR_MODE)
	if (!(attr->ia_valid & OCFS2_VALID_ATTRS))
		return 0;

	status = setattr_prepare(dentry, attr);
	if (status)
		return status;

	if (is_quota_modification(inode, attr)) {
		status = dquot_initialize(inode);
		if (status)
			return status;
	}
	size_change = S_ISREG(inode->i_mode) && attr->ia_valid & ATTR_SIZE;
	if (size_change) {
		status = ocfs2_rw_lock(inode, 1);
		if (status < 0) {
			mlog_errno(status);
			goto bail;
		}
	}

	had_lock = ocfs2_inode_lock_tracker(inode, &bh, 1, &oh);
	if (had_lock < 0) {
		status = had_lock;
		goto bail_unlock_rw;
	} else if (had_lock) {
		/*
		 * As far as we know, ocfs2_setattr() could only be the first
		 * VFS entry point in the call chain of recursive cluster
		 * locking issue.
		 *
		 * For instance:
		 * chmod_common()
		 *  notify_change()
		 *   ocfs2_setattr()
		 *    posix_acl_chmod()
		 *     ocfs2_iop_get_acl()
		 *
		 * But, we're not 100% sure if it's always true, because the
		 * ordering of the VFS entry points in the call chain is out
		 * of our control. So, we'd better dump the stack here to
		 * catch the other cases of recursive locking.
		 */
		mlog(ML_ERROR, "Another case of recursive locking:\n");
		dump_stack();
	}
	inode_locked = 1;

	if (size_change) {
		status = inode_newsize_ok(inode, attr->ia_size);
		if (status)
			goto bail_unlock;

		inode_dio_wait(inode);

		if (i_size_read(inode) >= attr->ia_size) {
			if (ocfs2_should_order_data(inode)) {
				status = ocfs2_begin_ordered_truncate(inode,
								      attr->ia_size);
				if (status)
					goto bail_unlock;
			}
			status = ocfs2_truncate_file(inode, bh, attr->ia_size);
		} else
			status = ocfs2_extend_file(inode, bh, attr->ia_size);
		if (status < 0) {
			if (status != -ENOSPC)
				mlog_errno(status);
			status = -ENOSPC;
			goto bail_unlock;
		}
	}

	if ((attr->ia_valid & ATTR_UID && !uid_eq(attr->ia_uid, inode->i_uid)) ||
	    (attr->ia_valid & ATTR_GID && !gid_eq(attr->ia_gid, inode->i_gid))) {
		/*
		 * Gather pointers to quota structures so that allocation /
		 * freeing of quota structures happens here and not inside
		 * dquot_transfer() where we have problems with lock ordering
		 */
		if (attr->ia_valid & ATTR_UID && !uid_eq(attr->ia_uid, inode->i_uid)
		    && OCFS2_HAS_RO_COMPAT_FEATURE(sb,
		    OCFS2_FEATURE_RO_COMPAT_USRQUOTA)) {
			transfer_to[USRQUOTA] = dqget(sb, make_kqid_uid(attr->ia_uid));
			if (IS_ERR(transfer_to[USRQUOTA])) {
				status = PTR_ERR(transfer_to[USRQUOTA]);
				goto bail_unlock;
			}
		}
		if (attr->ia_valid & ATTR_GID && !gid_eq(attr->ia_gid, inode->i_gid)
		    && OCFS2_HAS_RO_COMPAT_FEATURE(sb,
		    OCFS2_FEATURE_RO_COMPAT_GRPQUOTA)) {
			transfer_to[GRPQUOTA] = dqget(sb, make_kqid_gid(attr->ia_gid));
			if (IS_ERR(transfer_to[GRPQUOTA])) {
				status = PTR_ERR(transfer_to[GRPQUOTA]);
				goto bail_unlock;
			}
		}
		handle = ocfs2_start_trans(osb, OCFS2_INODE_UPDATE_CREDITS +
					   2 * ocfs2_quota_trans_credits(sb));
		if (IS_ERR(handle)) {
			status = PTR_ERR(handle);
			mlog_errno(status);
			goto bail_unlock;
		}
		status = __dquot_transfer(inode, transfer_to);
		if (status < 0)
			goto bail_commit;
	} else {
		handle = ocfs2_start_trans(osb, OCFS2_INODE_UPDATE_CREDITS);
		if (IS_ERR(handle)) {
			status = PTR_ERR(handle);
			mlog_errno(status);
			goto bail_unlock;
		}
	}

	setattr_copy(inode, attr);
	mark_inode_dirty(inode);

	status = ocfs2_mark_inode_dirty(handle, inode, bh);
	if (status < 0)
		mlog_errno(status);

bail_commit:
	ocfs2_commit_trans(osb, handle);
bail_unlock:
	if (status && inode_locked) {
		ocfs2_inode_unlock_tracker(inode, 1, &oh, had_lock);
		inode_locked = 0;
	}
bail_unlock_rw:
	if (size_change)
		ocfs2_rw_unlock(inode, 1);
bail:

	/* Release quota pointers in case we acquired them */
	for (qtype = 0; qtype < OCFS2_MAXQUOTAS; qtype++)
		dqput(transfer_to[qtype]);

	if (!status && attr->ia_valid & ATTR_MODE) {
		status = ocfs2_acl_chmod(inode, bh);
		if (status < 0)
			mlog_errno(status);
	}
	if (inode_locked)
		ocfs2_inode_unlock_tracker(inode, 1, &oh, had_lock);

	brelse(bh);
	return status;
}