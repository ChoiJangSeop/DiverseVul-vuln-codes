archive_write_disk_set_acls(struct archive *a, int fd, const char *name,
    struct archive_acl *abstract_acl, __LA_MODE_T mode)
{
	int		ret = ARCHIVE_OK;

	(void)mode;	/* UNUSED */

	if ((archive_acl_types(abstract_acl)
	    & ARCHIVE_ENTRY_ACL_TYPE_POSIX1E) != 0) {
		/* Solaris writes POSIX.1e access and default ACLs together */
		ret = set_acl(a, fd, name, abstract_acl,
		    ARCHIVE_ENTRY_ACL_TYPE_POSIX1E, "posix1e");

		/* Simultaneous POSIX.1e and NFSv4 is not supported */
		return (ret);
	}
#if ARCHIVE_ACL_SUNOS_NFS4
	else if ((archive_acl_types(abstract_acl) &
	    ARCHIVE_ENTRY_ACL_TYPE_NFS4) != 0) {
		ret = set_acl(a, fd, name, abstract_acl,
		    ARCHIVE_ENTRY_ACL_TYPE_NFS4, "nfs4");
	}
#endif
	return (ret);
}