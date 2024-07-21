SMBC_attr_server(TALLOC_CTX *ctx,
                 SMBCCTX *context,
                 const char *server,
                 uint16_t port,
                 const char *share,
                 char **pp_workgroup,
                 char **pp_username,
                 char **pp_password)
{
        int flags;
	struct cli_state *ipc_cli = NULL;
	struct rpc_pipe_client *pipe_hnd = NULL;
        NTSTATUS nt_status;
	SMBCSRV *srv=NULL;
	SMBCSRV *ipc_srv=NULL;

	/*
	 * Use srv->cli->desthost and srv->cli->share instead of
	 * server and share below to connect to the actual share,
	 * i.e., a normal share or a referred share from
	 * 'msdfs proxy' share.
	 */
	srv = SMBC_server(ctx, context, true, server, port, share,
			pp_workgroup, pp_username, pp_password);
	if (!srv) {
		return NULL;
	}
	server = smbXcli_conn_remote_name(srv->cli->conn);
	share = srv->cli->share;

        /*
         * See if we've already created this special connection.  Reference
         * our "special" share name '*IPC$', which is an impossible real share
         * name due to the leading asterisk.
         */
        ipc_srv = SMBC_find_server(ctx, context, server, "*IPC$",
                                   pp_workgroup, pp_username, pp_password);
        if (!ipc_srv) {

                /* We didn't find a cached connection.  Get the password */
		if (!*pp_password || (*pp_password)[0] == '\0') {
                        /* ... then retrieve it now. */
			SMBC_call_auth_fn(ctx, context, server, share,
                                          pp_workgroup,
                                          pp_username,
                                          pp_password);
			if (!*pp_workgroup || !*pp_username || !*pp_password) {
				errno = ENOMEM;
				return NULL;
			}
                }

                flags = 0;
                if (smbc_getOptionUseKerberos(context)) {
                        flags |= CLI_FULL_CONNECTION_USE_KERBEROS;
                }
                if (smbc_getOptionUseCCache(context)) {
                        flags |= CLI_FULL_CONNECTION_USE_CCACHE;
                }

                nt_status = cli_full_connection(&ipc_cli,
						lp_netbios_name(), server,
						NULL, 0, "IPC$", "?????",
						*pp_username,
						*pp_workgroup,
						*pp_password,
						flags,
						SMB_SIGNING_DEFAULT);
                if (! NT_STATUS_IS_OK(nt_status)) {
                        DEBUG(1,("cli_full_connection failed! (%s)\n",
                                 nt_errstr(nt_status)));
                        errno = ENOTSUP;
                        return NULL;
                }

		if (context->internal->smb_encryption_level) {
			/* Attempt UNIX smb encryption. */
			if (!NT_STATUS_IS_OK(cli_force_encryption(ipc_cli,
                                                                  *pp_username,
                                                                  *pp_password,
                                                                  *pp_workgroup))) {

				/*
				 * context->smb_encryption_level ==
				 * 1 means don't fail if encryption can't be
				 * negotiated, == 2 means fail if encryption
				 * can't be negotiated.
				 */

				DEBUG(4,(" SMB encrypt failed on IPC$\n"));

				if (context->internal->smb_encryption_level == 2) {
		                        cli_shutdown(ipc_cli);
					errno = EPERM;
					return NULL;
				}
			}
			DEBUG(4,(" SMB encrypt ok on IPC$\n"));
		}

                ipc_srv = SMB_MALLOC_P(SMBCSRV);
                if (!ipc_srv) {
                        errno = ENOMEM;
                        cli_shutdown(ipc_cli);
                        return NULL;
                }

                ZERO_STRUCTP(ipc_srv);
                ipc_srv->cli = ipc_cli;

                nt_status = cli_rpc_pipe_open_noauth(
			ipc_srv->cli, &ndr_table_lsarpc, &pipe_hnd);
                if (!NT_STATUS_IS_OK(nt_status)) {
                        DEBUG(1, ("cli_nt_session_open fail!\n"));
                        errno = ENOTSUP;
                        cli_shutdown(ipc_srv->cli);
                        free(ipc_srv);
                        return NULL;
                }

                /*
                 * Some systems don't support
                 * SEC_FLAG_MAXIMUM_ALLOWED, but NT sends 0x2000000
                 * so we might as well do it too.
                 */

                nt_status = rpccli_lsa_open_policy(
                        pipe_hnd,
                        talloc_tos(),
                        True,
                        GENERIC_EXECUTE_ACCESS,
                        &ipc_srv->pol);

                if (!NT_STATUS_IS_OK(nt_status)) {
                        errno = SMBC_errno(context, ipc_srv->cli);
                        cli_shutdown(ipc_srv->cli);
                        free(ipc_srv);
                        return NULL;
                }

                /* now add it to the cache (internal or external) */

                errno = 0;      /* let cache function set errno if it likes */
                if (smbc_getFunctionAddCachedServer(context)(context, ipc_srv,
                                                             server,
                                                             "*IPC$",
                                                             *pp_workgroup,
                                                             *pp_username)) {
                        DEBUG(3, (" Failed to add server to cache\n"));
                        if (errno == 0) {
                                errno = ENOMEM;
                        }
                        cli_shutdown(ipc_srv->cli);
                        free(ipc_srv);
                        return NULL;
                }

                DLIST_ADD(context->internal->servers, ipc_srv);
        }

        return ipc_srv;
}