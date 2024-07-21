SMBC_server_internal(TALLOC_CTX *ctx,
            SMBCCTX *context,
            bool connect_if_not_found,
            const char *server,
            uint16_t port,
            const char *share,
            char **pp_workgroup,
            char **pp_username,
            char **pp_password,
	    bool *in_cache)
{
	SMBCSRV *srv=NULL;
	char *workgroup = NULL;
	struct cli_state *c = NULL;
	const char *server_n = server;
        int is_ipc = (share != NULL && strcmp(share, "IPC$") == 0);
	uint32_t fs_attrs = 0;
        const char *username_used;
 	NTSTATUS status;
	char *newserver, *newshare;
	int flags = 0;
	struct smbXcli_tcon *tcon = NULL;

	ZERO_STRUCT(c);
	*in_cache = false;

	if (server[0] == 0) {
		errno = EPERM;
		return NULL;
	}

        /* Look for a cached connection */
        srv = SMBC_find_server(ctx, context, server, share,
                               pp_workgroup, pp_username, pp_password);

        /*
         * If we found a connection and we're only allowed one share per
         * server...
         */
        if (srv &&
	    share != NULL && *share != '\0' &&
            smbc_getOptionOneSharePerServer(context)) {

                /*
                 * ... then if there's no current connection to the share,
                 * connect to it.  SMBC_find_server(), or rather the function
                 * pointed to by context->get_cached_srv_fn which
                 * was called by SMBC_find_server(), will have issued a tree
                 * disconnect if the requested share is not the same as the
                 * one that was already connected.
                 */

		/*
		 * Use srv->cli->desthost and srv->cli->share instead of
		 * server and share below to connect to the actual share,
		 * i.e., a normal share or a referred share from
		 * 'msdfs proxy' share.
		 */
                if (!cli_state_has_tcon(srv->cli)) {
                        /* Ensure we have accurate auth info */
			SMBC_call_auth_fn(ctx, context,
					  smbXcli_conn_remote_name(srv->cli->conn),
					  srv->cli->share,
                                          pp_workgroup,
                                          pp_username,
                                          pp_password);

			if (!*pp_workgroup || !*pp_username || !*pp_password) {
				errno = ENOMEM;
				cli_shutdown(srv->cli);
				srv->cli = NULL;
				smbc_getFunctionRemoveCachedServer(context)(context,
                                                                            srv);
				return NULL;
			}

			/*
			 * We don't need to renegotiate encryption
			 * here as the encryption context is not per
			 * tid.
			 */

			status = cli_tree_connect(srv->cli,
						  srv->cli->share,
						  "?????",
						  *pp_password,
						  strlen(*pp_password)+1);
			if (!NT_STATUS_IS_OK(status)) {
                                errno = map_errno_from_nt_status(status);
                                cli_shutdown(srv->cli);
				srv->cli = NULL;
                                smbc_getFunctionRemoveCachedServer(context)(context,
                                                                            srv);
                                srv = NULL;
                        }

                        /* Determine if this share supports case sensitivity */
                        if (is_ipc) {
                                DEBUG(4,
                                      ("IPC$ so ignore case sensitivity\n"));
                                status = NT_STATUS_OK;
                        } else {
                                status = cli_get_fs_attr_info(c, &fs_attrs);
                        }

                        if (!NT_STATUS_IS_OK(status)) {
                                DEBUG(4, ("Could not retrieve "
                                          "case sensitivity flag: %s.\n",
                                          nt_errstr(status)));

                                /*
                                 * We can't determine the case sensitivity of
                                 * the share. We have no choice but to use the
                                 * user-specified case sensitivity setting.
                                 */
                                if (smbc_getOptionCaseSensitive(context)) {
                                        cli_set_case_sensitive(c, True);
                                } else {
                                        cli_set_case_sensitive(c, False);
                                }
                        } else if (!is_ipc) {
                                DEBUG(4,
                                      ("Case sensitive: %s\n",
                                       (fs_attrs & FILE_CASE_SENSITIVE_SEARCH
                                        ? "True"
                                        : "False")));
                                cli_set_case_sensitive(
                                        c,
                                        (fs_attrs & FILE_CASE_SENSITIVE_SEARCH
                                         ? True
                                         : False));
                        }

                        /*
                         * Regenerate the dev value since it's based on both
                         * server and share
                         */
                        if (srv) {
				const char *remote_name =
					smbXcli_conn_remote_name(srv->cli->conn);

				srv->dev = (dev_t)(str_checksum(remote_name) ^
                                                   str_checksum(srv->cli->share));
                        }
                }
        }

        /* If we have a connection... */
        if (srv) {

                /* ... then we're done here.  Give 'em what they came for. */
		*in_cache = true;
                goto done;
        }

        /* If we're not asked to connect when a connection doesn't exist... */
        if (! connect_if_not_found) {
                /* ... then we're done here. */
                return NULL;
        }

	if (!*pp_workgroup || !*pp_username || !*pp_password) {
		errno = ENOMEM;
		return NULL;
	}

	DEBUG(4,("SMBC_server: server_n=[%s] server=[%s]\n", server_n, server));

	DEBUG(4,(" -> server_n=[%s] server=[%s]\n", server_n, server));

	status = NT_STATUS_UNSUCCESSFUL;

	if (smbc_getOptionUseKerberos(context)) {
		flags |= CLI_FULL_CONNECTION_USE_KERBEROS;
	}

	if (smbc_getOptionFallbackAfterKerberos(context)) {
		flags |= CLI_FULL_CONNECTION_FALLBACK_AFTER_KERBEROS;
	}

	if (smbc_getOptionUseCCache(context)) {
		flags |= CLI_FULL_CONNECTION_USE_CCACHE;
	}

	if (smbc_getOptionUseNTHash(context)) {
		flags |= CLI_FULL_CONNECTION_USE_NT_HASH;
	}

	if (port == 0) {
	        if (share == NULL || *share == '\0' || is_ipc) {
			/*
			 * Try 139 first for IPC$
			 */
			status = cli_connect_nb(server_n, NULL, NBT_SMB_PORT, 0x20,
					smbc_getNetbiosName(context),
					SMB_SIGNING_DEFAULT, flags, &c);
		}
	}

	if (!NT_STATUS_IS_OK(status)) {
		/*
		 * No IPC$ or 139 did not work
		 */
		status = cli_connect_nb(server_n, NULL, port, 0x20,
					smbc_getNetbiosName(context),
					SMB_SIGNING_DEFAULT, flags, &c);
	}

	if (!NT_STATUS_IS_OK(status)) {
		errno = map_errno_from_nt_status(status);
		return NULL;
	}

	cli_set_timeout(c, smbc_getTimeout(context));

	status = smbXcli_negprot(c->conn, c->timeout,
				 lp_client_min_protocol(),
				 lp_client_max_protocol());
	if (!NT_STATUS_IS_OK(status)) {
		cli_shutdown(c);
		errno = ETIMEDOUT;
		return NULL;
	}

	if (smbXcli_conn_protocol(c->conn) >= PROTOCOL_SMB2_02) {
		/* Ensure we ask for some initial credits. */
		smb2cli_conn_set_max_credits(c->conn, DEFAULT_SMB2_MAX_CREDITS);
	}

        username_used = *pp_username;

	if (!NT_STATUS_IS_OK(cli_session_setup(c, username_used,
					       *pp_password,
                                               strlen(*pp_password),
					       *pp_password,
                                               strlen(*pp_password),
					       *pp_workgroup))) {

                /* Failed.  Try an anonymous login, if allowed by flags. */
                username_used = "";

                if (smbc_getOptionNoAutoAnonymousLogin(context) ||
                    !NT_STATUS_IS_OK(cli_session_setup(c, username_used,
                                                       *pp_password, 1,
                                                       *pp_password, 0,
                                                       *pp_workgroup))) {

                        cli_shutdown(c);
                        errno = EPERM;
                        return NULL;
                }
	}

	DEBUG(4,(" session setup ok\n"));

	/* here's the fun part....to support 'msdfs proxy' shares
	   (on Samba or windows) we have to issues a TRANS_GET_DFS_REFERRAL
	   here before trying to connect to the original share.
	   cli_check_msdfs_proxy() will fail if it is a normal share. */

	if (smbXcli_conn_dfs_supported(c->conn) &&
			cli_check_msdfs_proxy(ctx, c, share,
				&newserver, &newshare,
				/* FIXME: cli_check_msdfs_proxy() does
				   not support smbc_smb_encrypt_level type */
				context->internal->smb_encryption_level ?
					true : false,
				*pp_username,
				*pp_password,
				*pp_workgroup)) {
		cli_shutdown(c);
		srv = SMBC_server_internal(ctx, context, connect_if_not_found,
				newserver, port, newshare, pp_workgroup,
				pp_username, pp_password, in_cache);
		TALLOC_FREE(newserver);
		TALLOC_FREE(newshare);
		return srv;
	}

	/* must be a normal share */

	status = cli_tree_connect(c, share, "?????", *pp_password,
				  strlen(*pp_password)+1);
	if (!NT_STATUS_IS_OK(status)) {
		errno = map_errno_from_nt_status(status);
		cli_shutdown(c);
		return NULL;
	}

	DEBUG(4,(" tconx ok\n"));

	if (smbXcli_conn_protocol(c->conn) >= PROTOCOL_SMB2_02) {
		tcon = c->smb2.tcon;
	} else {
		tcon = c->smb1.tcon;
	}

        /* Determine if this share supports case sensitivity */
	if (is_ipc) {
                DEBUG(4, ("IPC$ so ignore case sensitivity\n"));
                status = NT_STATUS_OK;
        } else {
                status = cli_get_fs_attr_info(c, &fs_attrs);
        }

        if (!NT_STATUS_IS_OK(status)) {
                DEBUG(4, ("Could not retrieve case sensitivity flag: %s.\n",
                          nt_errstr(status)));

                /*
                 * We can't determine the case sensitivity of the share. We
                 * have no choice but to use the user-specified case
                 * sensitivity setting.
                 */
                if (smbc_getOptionCaseSensitive(context)) {
                        cli_set_case_sensitive(c, True);
                } else {
                        cli_set_case_sensitive(c, False);
                }
	} else if (!is_ipc) {
                DEBUG(4, ("Case sensitive: %s\n",
                          (fs_attrs & FILE_CASE_SENSITIVE_SEARCH
                           ? "True"
                           : "False")));
		smbXcli_tcon_set_fs_attributes(tcon, fs_attrs);
        }

	if (context->internal->smb_encryption_level) {
		/* Attempt UNIX smb encryption. */
		if (!NT_STATUS_IS_OK(cli_force_encryption(c,
                                                          username_used,
                                                          *pp_password,
                                                          *pp_workgroup))) {

			/*
			 * context->smb_encryption_level == 1
			 * means don't fail if encryption can't be negotiated,
			 * == 2 means fail if encryption can't be negotiated.
			 */

			DEBUG(4,(" SMB encrypt failed\n"));

			if (context->internal->smb_encryption_level == 2) {
	                        cli_shutdown(c);
				errno = EPERM;
				return NULL;
			}
		}
		DEBUG(4,(" SMB encrypt ok\n"));
	}

	/*
	 * Ok, we have got a nice connection
	 * Let's allocate a server structure.
	 */

	srv = SMB_MALLOC_P(SMBCSRV);
	if (!srv) {
		cli_shutdown(c);
		errno = ENOMEM;
		return NULL;
	}

	ZERO_STRUCTP(srv);
	srv->cli = c;
	srv->dev = (dev_t)(str_checksum(server) ^ str_checksum(share));
        srv->no_pathinfo = False;
        srv->no_pathinfo2 = False;
	srv->no_pathinfo3 = False;
        srv->no_nt_session = False;

done:
	if (!pp_workgroup || !*pp_workgroup || !**pp_workgroup) {
		workgroup = talloc_strdup(ctx, smbc_getWorkgroup(context));
	} else {
		workgroup = *pp_workgroup;
	}
	if(!workgroup) {
		if (c != NULL) {
			cli_shutdown(c);
		}
		SAFE_FREE(srv);
		return NULL;
	}

	/* set the credentials to make DFS work */
	smbc_set_credentials_with_fallback(context,
					   workgroup,
				    	   *pp_username,
				   	   *pp_password);

	return srv;
}