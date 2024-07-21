archive_write_disk_set_acls(struct archive *a, int fd, const char *name,
    struct archive_acl *abstract_acl, __LA_MODE_T mode)
{
	int		ret = ARCHIVE_OK;

	(void)mode;	/* UNUSED */

	if ((archive_acl_types(abstract_acl)
	    & ARCHIVE_ENTRY_ACL_TYPE_POSIX1E) != 0) {
		if ((archive_acl_types(abstract_acl)
		    & ARCHIVE_ENTRY_ACL_TYPE_ACCESS) != 0) {
			ret = set_acl(a, fd, name, abstract_acl,
			    ARCHIVE_ENTRY_ACL_TYPE_ACCESS, "access");
			if (ret != ARCHIVE_OK)
				return (ret);
		}
		if ((archive_acl_types(abstract_acl)
		    & ARCHIVE_ENTRY_ACL_TYPE_DEFAULT) != 0)
			ret = set_acl(a, fd, name, abstract_acl,
			    ARCHIVE_ENTRY_ACL_TYPE_DEFAULT, "default");

		/* Simultaneous POSIX.1e and NFSv4 is not supported */
		return (ret);
	}
#if ARCHIVE_ACL_FREEBSD_NFS4
	else if ((archive_acl_types(abstract_acl) &
	    ARCHIVE_ENTRY_ACL_TYPE_NFS4) != 0) {
		ret = set_acl(a, fd, name, abstract_acl,
		    ARCHIVE_ENTRY_ACL_TYPE_NFS4, "nfs4");
	}
#endif
	return (ret);
}