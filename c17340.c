zfs_fuid_map_id(zfsvfs_t *zfsvfs, uint64_t fuid,
    cred_t *cr, zfs_fuid_type_t type)
{
#ifdef HAVE_KSID
	uint32_t index = FUID_INDEX(fuid);
	const char *domain;
	uid_t id;

	if (index == 0)
		return (fuid);

	domain = zfs_fuid_find_by_idx(zfsvfs, index);
	ASSERT(domain != NULL);

	if (type == ZFS_OWNER || type == ZFS_ACE_USER) {
		(void) kidmap_getuidbysid(crgetzone(cr), domain,
		    FUID_RID(fuid), &id);
	} else {
		(void) kidmap_getgidbysid(crgetzone(cr), domain,
		    FUID_RID(fuid), &id);
	}
	return (id);
#else
	/*
	 * The Linux port only supports POSIX IDs, use the passed id.
	 */
	return (fuid);
#endif /* HAVE_KSID */
}