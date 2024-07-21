connection_struct *make_connection_snum(struct smbd_server_connection *sconn,
					int snum, user_struct *vuser,
					DATA_BLOB password,
					const char *pdev,
					NTSTATUS *pstatus)
{
	connection_struct *conn;
	struct smb_filename *smb_fname_cpath = NULL;
	fstring dev;
	int ret;
	char addr[INET6_ADDRSTRLEN];
	bool on_err_call_dis_hook = false;
	NTSTATUS status;

	fstrcpy(dev, pdev);

	if (NT_STATUS_IS_ERR(*pstatus = share_sanity_checks(snum, dev))) {
		return NULL;
	}	

	conn = conn_new(sconn);
	if (!conn) {
		DEBUG(0,("Couldn't find free connection.\n"));
		*pstatus = NT_STATUS_INSUFFICIENT_RESOURCES;
		return NULL;
	}

	conn->params->service = snum;

	status = create_connection_server_info(sconn,
		conn, snum, vuser ? vuser->server_info : NULL, password,
		&conn->server_info);

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(1, ("create_connection_server_info failed: %s\n",
			  nt_errstr(status)));
		*pstatus = status;
		conn_free(conn);
		return NULL;
	}

	if ((lp_guest_only(snum)) || (lp_security() == SEC_SHARE)) {
		conn->force_user = true;
	}

	add_session_user(sconn, conn->server_info->unix_name);

	safe_strcpy(conn->client_address,
			client_addr(get_client_fd(),addr,sizeof(addr)), 
			sizeof(conn->client_address)-1);
	conn->num_files_open = 0;
	conn->lastused = conn->lastused_count = time(NULL);
	conn->used = True;
	conn->printer = (strncmp(dev,"LPT",3) == 0);
	conn->ipc = ( (strncmp(dev,"IPC",3) == 0) ||
		      ( lp_enable_asu_support() && strequal(dev,"ADMIN$")) );

	/* Case options for the share. */
	if (lp_casesensitive(snum) == Auto) {
		/* We will be setting this per packet. Set to be case
		 * insensitive for now. */
		conn->case_sensitive = False;
	} else {
		conn->case_sensitive = (bool)lp_casesensitive(snum);
	}

	conn->case_preserve = lp_preservecase(snum);
	conn->short_case_preserve = lp_shortpreservecase(snum);

	conn->encrypt_level = lp_smb_encrypt(snum);

	conn->veto_list = NULL;
	conn->hide_list = NULL;
	conn->veto_oplock_list = NULL;
	conn->aio_write_behind_list = NULL;

	conn->read_only = lp_readonly(SNUM(conn));
	conn->admin_user = False;

	if (*lp_force_user(snum)) {

		/*
		 * Replace conn->server_info with a completely faked up one
		 * from the username we are forced into :-)
		 */

		char *fuser;
		struct auth_serversupplied_info *forced_serverinfo;

		fuser = talloc_string_sub(conn, lp_force_user(snum), "%S",
					  lp_servicename(snum));
		if (fuser == NULL) {
			conn_free(conn);
			*pstatus = NT_STATUS_NO_MEMORY;
			return NULL;
		}

		status = make_serverinfo_from_username(
			conn, fuser, conn->server_info->guest,
			&forced_serverinfo);
		if (!NT_STATUS_IS_OK(status)) {
			conn_free(conn);
			*pstatus = status;
			return NULL;
		}

		TALLOC_FREE(conn->server_info);
		conn->server_info = forced_serverinfo;

		conn->force_user = True;
		DEBUG(3,("Forced user %s\n", fuser));
	}

	/*
	 * If force group is true, then override
	 * any groupid stored for the connecting user.
	 */

	if (*lp_force_group(snum)) {

		status = find_forced_group(
			conn->force_user, snum, conn->server_info->unix_name,
			&conn->server_info->ptok->user_sids[1],
			&conn->server_info->utok.gid);

		if (!NT_STATUS_IS_OK(status)) {
			conn_free(conn);
			*pstatus = status;
			return NULL;
		}

		/*
		 * We need to cache this gid, to use within
 		 * change_to_user() separately from the conn->server_info
 		 * struct. We only use conn->server_info directly if
 		 * "force_user" was set.
 		 */
		conn->force_group_gid = conn->server_info->utok.gid;
	}

	conn->vuid = (vuser != NULL) ? vuser->vuid : UID_FIELD_INVALID;

	{
		char *s = talloc_sub_advanced(talloc_tos(),
					lp_servicename(SNUM(conn)),
					conn->server_info->unix_name,
					conn->connectpath,
					conn->server_info->utok.gid,
					conn->server_info->sanitized_username,
					pdb_get_domain(conn->server_info->sam_account),
					lp_pathname(snum));
		if (!s) {
			conn_free(conn);
			*pstatus = NT_STATUS_NO_MEMORY;
			return NULL;
		}

		if (!set_conn_connectpath(conn,s)) {
			TALLOC_FREE(s);
			conn_free(conn);
			*pstatus = NT_STATUS_NO_MEMORY;
			return NULL;
		}
		DEBUG(3,("Connect path is '%s' for service [%s]\n",s,
			 lp_servicename(snum)));
		TALLOC_FREE(s);
	}

	/*
	 * New code to check if there's a share security descripter
	 * added from NT server manager. This is done after the
	 * smb.conf checks are done as we need a uid and token. JRA.
	 *
	 */

	{
		bool can_write = False;

		can_write = share_access_check(conn->server_info->ptok,
					       lp_servicename(snum),
					       FILE_WRITE_DATA);

		if (!can_write) {
			if (!share_access_check(conn->server_info->ptok,
						lp_servicename(snum),
						FILE_READ_DATA)) {
				/* No access, read or write. */
				DEBUG(0,("make_connection: connection to %s "
					 "denied due to security "
					 "descriptor.\n",
					  lp_servicename(snum)));
				conn_free(conn);
				*pstatus = NT_STATUS_ACCESS_DENIED;
				return NULL;
			} else {
				conn->read_only = True;
			}
		}
	}
	/* Initialise VFS function pointers */

	if (!smbd_vfs_init(conn)) {
		DEBUG(0, ("vfs_init failed for service %s\n",
			  lp_servicename(snum)));
		conn_free(conn);
		*pstatus = NT_STATUS_BAD_NETWORK_NAME;
		return NULL;
	}

	/*
	 * If widelinks are disallowed we need to canonicalise the connect
	 * path here to ensure we don't have any symlinks in the
	 * connectpath. We will be checking all paths on this connection are
	 * below this directory. We must do this after the VFS init as we
	 * depend on the realpath() pointer in the vfs table. JRA.
	 */
	if (!lp_widelinks(snum)) {
		if (!canonicalize_connect_path(conn)) {
			DEBUG(0, ("canonicalize_connect_path failed "
			"for service %s, path %s\n",
				lp_servicename(snum),
				conn->connectpath));
			conn_free(conn);
			*pstatus = NT_STATUS_BAD_NETWORK_NAME;
			return NULL;
		}
	}

	if ((!conn->printer) && (!conn->ipc)) {
		conn->notify_ctx = notify_init(conn, server_id_self(),
					       smbd_messaging_context(),
					       smbd_event_context(),
					       conn);
	}

/* ROOT Activities: */	
	/*
	 * Enforce the max connections parameter.
	 */

	if ((lp_max_connections(snum) > 0)
	    && (count_current_connections(lp_servicename(SNUM(conn)), True) >=
		lp_max_connections(snum))) {

		DEBUG(1, ("Max connections (%d) exceeded for %s\n",
			  lp_max_connections(snum), lp_servicename(snum)));
		conn_free(conn);
		*pstatus = NT_STATUS_INSUFFICIENT_RESOURCES;
		return NULL;
	}  

	/*
	 * Get us an entry in the connections db
	 */
	if (!claim_connection(conn, lp_servicename(snum), 0)) {
		DEBUG(1, ("Could not store connections entry\n"));
		conn_free(conn);
		*pstatus = NT_STATUS_INTERNAL_DB_ERROR;
		return NULL;
	}  

	/* Preexecs are done here as they might make the dir we are to ChDir
	 * to below */
	/* execute any "root preexec = " line */
	if (*lp_rootpreexec(snum)) {
		char *cmd = talloc_sub_advanced(talloc_tos(),
					lp_servicename(SNUM(conn)),
					conn->server_info->unix_name,
					conn->connectpath,
					conn->server_info->utok.gid,
					conn->server_info->sanitized_username,
					pdb_get_domain(conn->server_info->sam_account),
					lp_rootpreexec(snum));
		DEBUG(5,("cmd=%s\n",cmd));
		ret = smbrun(cmd,NULL);
		TALLOC_FREE(cmd);
		if (ret != 0 && lp_rootpreexec_close(snum)) {
			DEBUG(1,("root preexec gave %d - failing "
				 "connection\n", ret));
			yield_connection(conn, lp_servicename(snum));
			conn_free(conn);
			*pstatus = NT_STATUS_ACCESS_DENIED;
			return NULL;
		}
	}

/* USER Activites: */
	if (!change_to_user(conn, conn->vuid)) {
		/* No point continuing if they fail the basic checks */
		DEBUG(0,("Can't become connected user!\n"));
		yield_connection(conn, lp_servicename(snum));
		conn_free(conn);
		*pstatus = NT_STATUS_LOGON_FAILURE;
		return NULL;
	}

	/* Remember that a different vuid can connect later without these
	 * checks... */
	
	/* Preexecs are done here as they might make the dir we are to ChDir
	 * to below */

	/* execute any "preexec = " line */
	if (*lp_preexec(snum)) {
		char *cmd = talloc_sub_advanced(talloc_tos(),
					lp_servicename(SNUM(conn)),
					conn->server_info->unix_name,
					conn->connectpath,
					conn->server_info->utok.gid,
					conn->server_info->sanitized_username,
					pdb_get_domain(conn->server_info->sam_account),
					lp_preexec(snum));
		ret = smbrun(cmd,NULL);
		TALLOC_FREE(cmd);
		if (ret != 0 && lp_preexec_close(snum)) {
			DEBUG(1,("preexec gave %d - failing connection\n",
				 ret));
			*pstatus = NT_STATUS_ACCESS_DENIED;
			goto err_root_exit;
		}
	}

#ifdef WITH_FAKE_KASERVER
	if (lp_afs_share(snum)) {
		afs_login(conn);
	}
#endif
	
	/* Add veto/hide lists */
	if (!IS_IPC(conn) && !IS_PRINT(conn)) {
		set_namearray( &conn->veto_list, lp_veto_files(snum));
		set_namearray( &conn->hide_list, lp_hide_files(snum));
		set_namearray( &conn->veto_oplock_list, lp_veto_oplocks(snum));
		set_namearray( &conn->aio_write_behind_list,
				lp_aio_write_behind(snum));
	}
	
	/* Invoke VFS make connection hook - do this before the VFS_STAT call
	   to allow any filesystems needing user credentials to initialize
	   themselves. */

	if (SMB_VFS_CONNECT(conn, lp_servicename(snum),
			    conn->server_info->unix_name) < 0) {
		DEBUG(0,("make_connection: VFS make connection failed!\n"));
		*pstatus = NT_STATUS_UNSUCCESSFUL;
		goto err_root_exit;
	}

	/* Any error exit after here needs to call the disconnect hook. */
	on_err_call_dis_hook = true;

	status = create_synthetic_smb_fname(talloc_tos(), conn->connectpath,
					    NULL, NULL, &smb_fname_cpath);
	if (!NT_STATUS_IS_OK(status)) {
		*pstatus = status;
		goto err_root_exit;
	}

	/* win2000 does not check the permissions on the directory
	   during the tree connect, instead relying on permission
	   check during individual operations. To match this behaviour
	   I have disabled this chdir check (tridge) */
	/* the alternative is just to check the directory exists */
	if ((ret = SMB_VFS_STAT(conn, smb_fname_cpath)) != 0 ||
	    !S_ISDIR(smb_fname_cpath->st.st_ex_mode)) {
		if (ret == 0 && !S_ISDIR(smb_fname_cpath->st.st_ex_mode)) {
			DEBUG(0,("'%s' is not a directory, when connecting to "
				 "[%s]\n", conn->connectpath,
				 lp_servicename(snum)));
		} else {
			DEBUG(0,("'%s' does not exist or permission denied "
				 "when connecting to [%s] Error was %s\n",
				 conn->connectpath, lp_servicename(snum),
				 strerror(errno) ));
		}
		*pstatus = NT_STATUS_BAD_NETWORK_NAME;
		goto err_root_exit;
	}

	string_set(&conn->origpath,conn->connectpath);

#if SOFTLINK_OPTIMISATION
	/* resolve any soft links early if possible */
	if (vfs_ChDir(conn,conn->connectpath) == 0) {
		TALLOC_CTX *ctx = talloc_tos();
		char *s = vfs_GetWd(ctx,s);
		if (!s) {
			*status = map_nt_error_from_unix(errno);
			goto err_root_exit;
		}
		if (!set_conn_connectpath(conn,s)) {
			*status = NT_STATUS_NO_MEMORY;
			goto err_root_exit;
		}
		vfs_ChDir(conn,conn->connectpath);
	}
#endif

	/* Figure out the characteristics of the underlying filesystem. This
	 * assumes that all the filesystem mounted withing a share path have
	 * the same characteristics, which is likely but not guaranteed.
	 */

	conn->fs_capabilities = SMB_VFS_FS_CAPABILITIES(conn, &conn->ts_res);

	/*
	 * Print out the 'connected as' stuff here as we need
	 * to know the effective uid and gid we will be using
	 * (at least initially).
	 */

	if( DEBUGLVL( IS_IPC(conn) ? 3 : 1 ) ) {
		dbgtext( "%s (%s) ", get_remote_machine_name(),
			 conn->client_address );
		dbgtext( "%s", srv_is_signing_active(smbd_server_conn) ? "signed " : "");
		dbgtext( "connect to service %s ", lp_servicename(snum) );
		dbgtext( "initially as user %s ",
			 conn->server_info->unix_name );
		dbgtext( "(uid=%d, gid=%d) ", (int)geteuid(), (int)getegid() );
		dbgtext( "(pid %d)\n", (int)sys_getpid() );
	}

	/* we've finished with the user stuff - go back to root */
	change_to_root_user();
	return(conn);

  err_root_exit:
	TALLOC_FREE(smb_fname_cpath);
	change_to_root_user();
	if (on_err_call_dis_hook) {
		/* Call VFS disconnect hook */
		SMB_VFS_DISCONNECT(conn);
	}
	yield_connection(conn, lp_servicename(snum));
	conn_free(conn);
	return NULL;
}