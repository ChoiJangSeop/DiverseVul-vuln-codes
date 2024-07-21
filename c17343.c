zfs_groupmember(zfsvfs_t *zfsvfs, uint64_t id, cred_t *cr)
{
#ifdef HAVE_KSID
	ksid_t		*ksid = crgetsid(cr, KSID_GROUP);
	ksidlist_t	*ksidlist = crgetsidlist(cr);
	uid_t		gid;

	if (ksid && ksidlist) {
		int		i;
		ksid_t		*ksid_groups;
		uint32_t	idx = FUID_INDEX(id);
		uint32_t	rid = FUID_RID(id);

		ksid_groups = ksidlist->ksl_sids;

		for (i = 0; i != ksidlist->ksl_nsid; i++) {
			if (idx == 0) {
				if (id != IDMAP_WK_CREATOR_GROUP_GID &&
				    id == ksid_groups[i].ks_id) {
					return (B_TRUE);
				}
			} else {
				const char *domain;

				domain = zfs_fuid_find_by_idx(zfsvfs, idx);
				ASSERT(domain != NULL);

				if (strcmp(domain,
				    IDMAP_WK_CREATOR_SID_AUTHORITY) == 0)
					return (B_FALSE);

				if ((strcmp(domain,
				    ksid_groups[i].ks_domain->kd_name) == 0) &&
				    rid == ksid_groups[i].ks_rid)
					return (B_TRUE);
			}
		}
	}

	/*
	 * Not found in ksidlist, check posix groups
	 */
	gid = zfs_fuid_map_id(zfsvfs, id, cr, ZFS_GROUP);
	return (groupmember(gid, cr));
#else
	return (B_TRUE);
#endif
}