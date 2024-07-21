static int get_file_caps(struct linux_binprm *bprm)
{
	struct dentry *dentry;
	int rc = 0;
	struct vfs_cap_data vcaps;
	struct inode *inode;

	if (bprm->file->f_vfsmnt->mnt_flags & MNT_NOSUID) {
		bprm_clear_caps(bprm);
		return 0;
	}

	dentry = dget(bprm->file->f_dentry);
	inode = dentry->d_inode;
	if (!inode->i_op || !inode->i_op->getxattr)
		goto out;

	rc = inode->i_op->getxattr(dentry, XATTR_NAME_CAPS, &vcaps,
				   XATTR_CAPS_SZ);
	if (rc == -ENODATA || rc == -EOPNOTSUPP) {
		/* no data, that's ok */
		rc = 0;
		goto out;
	}
	if (rc < 0)
		goto out;

	rc = cap_from_disk(&vcaps, bprm, rc);
	if (rc == -EINVAL)
		printk(KERN_NOTICE "%s: cap_from_disk returned %d for %s\n",
		       __func__, rc, bprm->filename);

out:
	dput(dentry);
	if (rc)
		bprm_clear_caps(bprm);

	return rc;
}