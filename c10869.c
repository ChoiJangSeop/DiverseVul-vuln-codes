set_richacl(struct archive *a, int fd, const char *name,
    struct archive_acl *abstract_acl, __LA_MODE_T mode,
    int ae_requested_type, const char *tname)
{
	int		 ae_type, ae_permset, ae_tag, ae_id;
	uid_t		 ae_uid;
	gid_t		 ae_gid;
	const char	*ae_name;
	int		 entries;
	int		 i;
	int		 ret;
	int		 e = 0;
	struct richacl  *richacl = NULL;
	struct richace  *richace;

	ret = ARCHIVE_OK;
	entries = archive_acl_reset(abstract_acl, ae_requested_type);
	if (entries == 0)
		return (ARCHIVE_OK);

	if (ae_requested_type != ARCHIVE_ENTRY_ACL_TYPE_NFS4) {
		errno = ENOENT;
		archive_set_error(a, errno, "Unsupported ACL type");
		return (ARCHIVE_FAILED);
	}

	richacl = richacl_alloc(entries);
	if (richacl == NULL) {
		archive_set_error(a, errno,
			"Failed to initialize RichACL working storage");
		return (ARCHIVE_FAILED);
	}

	e = 0;

	while (archive_acl_next(a, abstract_acl, ae_requested_type, &ae_type,
		   &ae_permset, &ae_tag, &ae_id, &ae_name) == ARCHIVE_OK) {
		richace = &(richacl->a_entries[e]);

		richace->e_flags = 0;
		richace->e_mask = 0;

		switch (ae_tag) {
		case ARCHIVE_ENTRY_ACL_USER:
			ae_uid = archive_write_disk_uid(a, ae_name, ae_id);
			richace->e_id = ae_uid;
			break;
		case ARCHIVE_ENTRY_ACL_GROUP:
			ae_gid = archive_write_disk_gid(a, ae_name, ae_id);
			richace->e_id = ae_gid;
			richace->e_flags |= RICHACE_IDENTIFIER_GROUP;
			break;
		case ARCHIVE_ENTRY_ACL_USER_OBJ:
			richace->e_flags |= RICHACE_SPECIAL_WHO;
			richace->e_id = RICHACE_OWNER_SPECIAL_ID;
			break;
		case ARCHIVE_ENTRY_ACL_GROUP_OBJ:
			richace->e_flags |= RICHACE_SPECIAL_WHO;
			richace->e_id = RICHACE_GROUP_SPECIAL_ID;
			break;
		case ARCHIVE_ENTRY_ACL_EVERYONE:
			richace->e_flags |= RICHACE_SPECIAL_WHO;
			richace->e_id = RICHACE_EVERYONE_SPECIAL_ID;
			break;
		default:
			archive_set_error(a, ARCHIVE_ERRNO_MISC,
			    "Unsupported ACL tag");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		switch (ae_type) {
			case ARCHIVE_ENTRY_ACL_TYPE_ALLOW:
				richace->e_type =
				    RICHACE_ACCESS_ALLOWED_ACE_TYPE;
				break;
			case ARCHIVE_ENTRY_ACL_TYPE_DENY:
				richace->e_type =
				    RICHACE_ACCESS_DENIED_ACE_TYPE;
				break;
			case ARCHIVE_ENTRY_ACL_TYPE_AUDIT:
			case ARCHIVE_ENTRY_ACL_TYPE_ALARM:
				break;
		default:
			archive_set_error(a, ARCHIVE_ERRNO_MISC,
			    "Unsupported ACL entry type");
			ret = ARCHIVE_FAILED;
			goto exit_free;
		}

		for (i = 0; i < acl_nfs4_perm_map_size; ++i) {
			if (ae_permset & acl_nfs4_perm_map[i].a_perm)
				richace->e_mask |= acl_nfs4_perm_map[i].p_perm;
		}

		for (i = 0; i < acl_nfs4_flag_map_size; ++i) {
			if (ae_permset &
			    acl_nfs4_flag_map[i].a_perm)
				richace->e_flags |= acl_nfs4_flag_map[i].p_perm;
		}
	e++;
	}

	/* Fill RichACL masks */
	_richacl_mode_to_masks(richacl, mode);

	if (fd >= 0) {
		if (richacl_set_fd(fd, richacl) == 0)
			ret = ARCHIVE_OK;
		else {
			if (errno == EOPNOTSUPP) {
				/* Filesystem doesn't support ACLs */
				ret = ARCHIVE_OK;
			} else {
				archive_set_error(a, errno,
				    "Failed to set richacl on fd: %s", tname);
				ret = ARCHIVE_WARN;
			}
		}
	} else if (richacl_set_file(name, richacl) != 0) {
		if (errno == EOPNOTSUPP) {
			/* Filesystem doesn't support ACLs */
			ret = ARCHIVE_OK;
		} else {
			archive_set_error(a, errno, "Failed to set richacl: %s",
			    tname);
			ret = ARCHIVE_WARN;
		}
	}
exit_free:
	richacl_free(richacl);
	return (ret);
}