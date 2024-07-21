zfs_fuid_create(zfsvfs_t *zfsvfs, uint64_t id, cred_t *cr,
    zfs_fuid_type_t type, zfs_fuid_info_t **fuidpp)
{
#ifdef HAVE_KSID
	const char *domain;
	char *kdomain;
	uint32_t fuid_idx = FUID_INDEX(id);
	uint32_t rid;
	idmap_stat status;
	uint64_t idx = 0;
	zfs_fuid_t *zfuid = NULL;
	zfs_fuid_info_t *fuidp = NULL;

	/*
	 * If POSIX ID, or entry is already a FUID then
	 * just return the id
	 *
	 * We may also be handed an already FUID'ized id via
	 * chmod.
	 */

	if (!zfsvfs->z_use_fuids || !IS_EPHEMERAL(id) || fuid_idx != 0)
		return (id);

	if (zfsvfs->z_replay) {
		fuidp = zfsvfs->z_fuid_replay;

		/*
		 * If we are passed an ephemeral id, but no
		 * fuid_info was logged then return NOBODY.
		 * This is most likely a result of idmap service
		 * not being available.
		 */
		if (fuidp == NULL)
			return (UID_NOBODY);

		VERIFY3U(type, >=, ZFS_OWNER);
		VERIFY3U(type, <=, ZFS_ACE_GROUP);

		switch (type) {
		case ZFS_ACE_USER:
		case ZFS_ACE_GROUP:
			zfuid = list_head(&fuidp->z_fuids);
			rid = FUID_RID(zfuid->z_logfuid);
			idx = FUID_INDEX(zfuid->z_logfuid);
			break;
		case ZFS_OWNER:
			rid = FUID_RID(fuidp->z_fuid_owner);
			idx = FUID_INDEX(fuidp->z_fuid_owner);
			break;
		case ZFS_GROUP:
			rid = FUID_RID(fuidp->z_fuid_group);
			idx = FUID_INDEX(fuidp->z_fuid_group);
			break;
		};
		domain = fuidp->z_domain_table[idx - 1];
	} else {
		if (type == ZFS_OWNER || type == ZFS_ACE_USER)
			status = kidmap_getsidbyuid(crgetzone(cr), id,
			    &domain, &rid);
		else
			status = kidmap_getsidbygid(crgetzone(cr), id,
			    &domain, &rid);

		if (status != 0) {
			/*
			 * When returning nobody we will need to
			 * make a dummy fuid table entry for logging
			 * purposes.
			 */
			rid = UID_NOBODY;
			domain = nulldomain;
		}
	}

	idx = zfs_fuid_find_by_domain(zfsvfs, domain, &kdomain, B_TRUE);

	if (!zfsvfs->z_replay)
		zfs_fuid_node_add(fuidpp, kdomain,
		    rid, idx, id, type);
	else if (zfuid != NULL) {
		list_remove(&fuidp->z_fuids, zfuid);
		kmem_free(zfuid, sizeof (zfs_fuid_t));
	}
	return (FUID_ENCODE(idx, rid));
#else
	/*
	 * The Linux port only supports POSIX IDs, use the passed id.
	 */
	return (id);
#endif
}