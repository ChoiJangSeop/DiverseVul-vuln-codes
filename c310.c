static bool set_canon_ace_list(files_struct *fsp, canon_ace *the_ace, bool default_ace, gid_t prim_gid, bool *pacl_set_support)
{
	connection_struct *conn = fsp->conn;
	bool ret = False;
	SMB_ACL_T the_acl = SMB_VFS_SYS_ACL_INIT(conn, (int)count_canon_ace_list(the_ace) + 1);
	canon_ace *p_ace;
	int i;
	SMB_ACL_ENTRY_T mask_entry;
	bool got_mask_entry = False;
	SMB_ACL_PERMSET_T mask_permset;
	SMB_ACL_TYPE_T the_acl_type = (default_ace ? SMB_ACL_TYPE_DEFAULT : SMB_ACL_TYPE_ACCESS);
	bool needs_mask = False;
	mode_t mask_perms = 0;

#if defined(POSIX_ACL_NEEDS_MASK)
	/* HP-UX always wants to have a mask (called "class" there). */
	needs_mask = True;
#endif

	if (the_acl == NULL) {

		if (!no_acl_syscall_error(errno)) {
			/*
			 * Only print this error message if we have some kind of ACL
			 * support that's not working. Otherwise we would always get this.
			 */
			DEBUG(0,("set_canon_ace_list: Unable to init %s ACL. (%s)\n",
				default_ace ? "default" : "file", strerror(errno) ));
		}
		*pacl_set_support = False;
		return False;
	}

	if( DEBUGLVL( 10 )) {
		dbgtext("set_canon_ace_list: setting ACL:\n");
		for (i = 0, p_ace = the_ace; p_ace; p_ace = p_ace->next, i++ ) {
			print_canon_ace( p_ace, i);
		}
	}

	for (i = 0, p_ace = the_ace; p_ace; p_ace = p_ace->next, i++ ) {
		SMB_ACL_ENTRY_T the_entry;
		SMB_ACL_PERMSET_T the_permset;

		/*
		 * ACLs only "need" an ACL_MASK entry if there are any named user or
		 * named group entries. But if there is an ACL_MASK entry, it applies
		 * to ACL_USER, ACL_GROUP, and ACL_GROUP_OBJ entries. Set the mask
		 * so that it doesn't deny (i.e., mask off) any permissions.
		 */

		if (p_ace->type == SMB_ACL_USER || p_ace->type == SMB_ACL_GROUP) {
			needs_mask = True;
			mask_perms |= p_ace->perms;
		} else if (p_ace->type == SMB_ACL_GROUP_OBJ) {
			mask_perms |= p_ace->perms;
		}

		/*
		 * Get the entry for this ACE.
		 */

		if (SMB_VFS_SYS_ACL_CREATE_ENTRY(conn, &the_acl, &the_entry) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to create entry %d. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		if (p_ace->type == SMB_ACL_MASK) {
			mask_entry = the_entry;
			got_mask_entry = True;
		}

		/*
		 * Ok - we now know the ACL calls should be working, don't
		 * allow fallback to chmod.
		 */

		*pacl_set_support = True;

		/*
		 * Initialise the entry from the canon_ace.
		 */

		/*
		 * First tell the entry what type of ACE this is.
		 */

		if (SMB_VFS_SYS_ACL_SET_TAG_TYPE(conn, the_entry, p_ace->type) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to set tag type on entry %d. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		/*
		 * Only set the qualifier (user or group id) if the entry is a user
		 * or group id ACE.
		 */

		if ((p_ace->type == SMB_ACL_USER) || (p_ace->type == SMB_ACL_GROUP)) {
			if (SMB_VFS_SYS_ACL_SET_QUALIFIER(conn, the_entry,(void *)&p_ace->unix_ug.uid) == -1) {
				DEBUG(0,("set_canon_ace_list: Failed to set qualifier on entry %d. (%s)\n",
					i, strerror(errno) ));
				goto fail;
			}
		}

		/*
		 * Convert the mode_t perms in the canon_ace to a POSIX permset.
		 */

		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, the_entry, &the_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to get permset on entry %d. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		if (map_acl_perms_to_permset(conn, p_ace->perms, &the_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to create permset for mode (%u) on entry %d. (%s)\n",
				(unsigned int)p_ace->perms, i, strerror(errno) ));
			goto fail;
		}

		/*
		 * ..and apply them to the entry.
		 */

		if (SMB_VFS_SYS_ACL_SET_PERMSET(conn, the_entry, the_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to add permset on entry %d. (%s)\n",
				i, strerror(errno) ));
			goto fail;
		}

		if( DEBUGLVL( 10 ))
			print_canon_ace( p_ace, i);

	}

	if (needs_mask && !got_mask_entry) {
		if (SMB_VFS_SYS_ACL_CREATE_ENTRY(conn, &the_acl, &mask_entry) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to create mask entry. (%s)\n", strerror(errno) ));
			goto fail;
		}

		if (SMB_VFS_SYS_ACL_SET_TAG_TYPE(conn, mask_entry, SMB_ACL_MASK) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to set tag type on mask entry. (%s)\n",strerror(errno) ));
			goto fail;
		}

		if (SMB_VFS_SYS_ACL_GET_PERMSET(conn, mask_entry, &mask_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to get mask permset. (%s)\n", strerror(errno) ));
			goto fail;
		}

		if (map_acl_perms_to_permset(conn, S_IRUSR|S_IWUSR|S_IXUSR, &mask_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to create mask permset. (%s)\n", strerror(errno) ));
			goto fail;
		}

		if (SMB_VFS_SYS_ACL_SET_PERMSET(conn, mask_entry, mask_permset) == -1) {
			DEBUG(0,("set_canon_ace_list: Failed to add mask permset. (%s)\n", strerror(errno) ));
			goto fail;
		}
	}

	/*
	 * Finally apply it to the file or directory.
	 */

	if(default_ace || fsp->is_directory || fsp->fh->fd == -1) {
		if (SMB_VFS_SYS_ACL_SET_FILE(conn, fsp->fsp_name, the_acl_type, the_acl) == -1) {
			/*
			 * Some systems allow all the above calls and only fail with no ACL support
			 * when attempting to apply the acl. HPUX with HFS is an example of this. JRA.
			 */
			if (no_acl_syscall_error(errno)) {
				*pacl_set_support = False;
			}

			if (acl_group_override(conn, prim_gid, fsp->fsp_name)) {
				int sret;

				DEBUG(5,("set_canon_ace_list: acl group control on and current user in file %s primary group.\n",
					fsp->fsp_name ));

				become_root();
				sret = SMB_VFS_SYS_ACL_SET_FILE(conn, fsp->fsp_name, the_acl_type, the_acl);
				unbecome_root();
				if (sret == 0) {
					ret = True;	
				}
			}

			if (ret == False) {
				DEBUG(2,("set_canon_ace_list: sys_acl_set_file type %s failed for file %s (%s).\n",
						the_acl_type == SMB_ACL_TYPE_DEFAULT ? "directory default" : "file",
						fsp->fsp_name, strerror(errno) ));
				goto fail;
			}
		}
	} else {
		if (SMB_VFS_SYS_ACL_SET_FD(fsp, the_acl) == -1) {
			/*
			 * Some systems allow all the above calls and only fail with no ACL support
			 * when attempting to apply the acl. HPUX with HFS is an example of this. JRA.
			 */
			if (no_acl_syscall_error(errno)) {
				*pacl_set_support = False;
			}

			if (acl_group_override(conn, prim_gid, fsp->fsp_name)) {
				int sret;

				DEBUG(5,("set_canon_ace_list: acl group control on and current user in file %s primary group.\n",
					fsp->fsp_name ));

				become_root();
				sret = SMB_VFS_SYS_ACL_SET_FD(fsp, the_acl);
				unbecome_root();
				if (sret == 0) {
					ret = True;
				}
			}

			if (ret == False) {
				DEBUG(2,("set_canon_ace_list: sys_acl_set_file failed for file %s (%s).\n",
						fsp->fsp_name, strerror(errno) ));
				goto fail;
			}
		}
	}

	ret = True;

  fail:

	if (the_acl != NULL) {
		SMB_VFS_SYS_ACL_FREE_ACL(conn, the_acl);
	}

	return ret;
}