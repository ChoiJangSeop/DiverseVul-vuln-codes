zfs_fastaccesschk_execute(znode_t *zdp, cred_t *cr)
{
	boolean_t owner = B_FALSE;
	boolean_t groupmbr = B_FALSE;
	boolean_t is_attr;
	uid_t uid = crgetuid(cr);

	if (zdp->z_pflags & ZFS_AV_QUARANTINED)
		return (1);

	is_attr = ((zdp->z_pflags & ZFS_XATTR) &&
	    (ZTOV(zdp)->v_type == VDIR));
	if (is_attr)
		return (1);

	if (zdp->z_pflags & ZFS_NO_EXECS_DENIED)
		return (0);

	mutex_enter(&zdp->z_acl_lock);
	if (FUID_INDEX(zdp->z_uid) != 0 || FUID_INDEX(zdp->z_gid) != 0) {
		goto out_slow;
	}

	if (uid == zdp->z_uid) {
		owner = B_TRUE;
		if (zdp->z_mode & S_IXUSR) {
			goto out;
		} else {
			goto out_slow;
		}
	}
	if (groupmember(zdp->z_gid, cr)) {
		groupmbr = B_TRUE;
		if (zdp->z_mode & S_IXGRP) {
			goto out;
		} else {
			goto out_slow;
		}
	}
	if (!owner && !groupmbr) {
		if (zdp->z_mode & S_IXOTH) {
			goto out;
		}
	}
out:
	mutex_exit(&zdp->z_acl_lock);
	return (0);
out_slow:
	mutex_exit(&zdp->z_acl_lock);
	return (1);
}