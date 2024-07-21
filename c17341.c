zfs_fuid_create_cred(zfsvfs_t *zfsvfs, zfs_fuid_type_t type,
    cred_t *cr, zfs_fuid_info_t **fuidp)
{
	uid_t		id;

	VERIFY(type == ZFS_OWNER || type == ZFS_GROUP);

	id = (type == ZFS_OWNER) ? crgetuid(cr) : crgetgid(cr);

	if (IS_EPHEMERAL(id))
		return ((type == ZFS_OWNER) ? UID_NOBODY : GID_NOBODY);

	return ((uint64_t)id);
}