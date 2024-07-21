static NTSTATUS do_connect(TALLOC_CTX *ctx,
					const char *server,
					const char *share,
					const struct user_auth_info *auth_info,
					bool show_sessetup,
					bool force_encrypt,
					int max_protocol,
					int port,
					int name_type,
					struct cli_state **pcli)
{
	struct cli_state *c = NULL;
	char *servicename;
	char *sharename;
	char *newserver, *newshare;
	const char *username;
	const char *password;
	const char *domain;
	NTSTATUS status;
	int flags = 0;

	/* make a copy so we don't modify the global string 'service' */
	servicename = talloc_strdup(ctx,share);
	if (!servicename) {
		return NT_STATUS_NO_MEMORY;
	}
	sharename = servicename;
	if (*sharename == '\\') {
		sharename += 2;
		if (server == NULL) {
			server = sharename;
		}
		sharename = strchr_m(sharename,'\\');
		if (!sharename) {
			return NT_STATUS_NO_MEMORY;
		}
		*sharename = 0;
		sharename++;
	}
	if (server == NULL) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (get_cmdline_auth_info_use_kerberos(auth_info)) {
		flags |= CLI_FULL_CONNECTION_USE_KERBEROS;
	}
	if (get_cmdline_auth_info_fallback_after_kerberos(auth_info)) {
		flags |= CLI_FULL_CONNECTION_FALLBACK_AFTER_KERBEROS;
	}
	if (get_cmdline_auth_info_use_ccache(auth_info)) {
		flags |= CLI_FULL_CONNECTION_USE_CCACHE;
	}
	if (get_cmdline_auth_info_use_pw_nt_hash(auth_info)) {
		flags |= CLI_FULL_CONNECTION_USE_NT_HASH;
	}

	status = cli_connect_nb(
		server, NULL, port, name_type, NULL,
		get_cmdline_auth_info_signing_state(auth_info),
		flags, &c);

	if (!NT_STATUS_IS_OK(status)) {
		d_printf("Connection to %s failed (Error %s)\n",
				server,
				nt_errstr(status));
		return status;
	}

	if (max_protocol == 0) {
		max_protocol = PROTOCOL_NT1;
	}
	DEBUG(4,(" session request ok\n"));

	status = smbXcli_negprot(c->conn, c->timeout,
				 lp_client_min_protocol(),
				 max_protocol);

	if (!NT_STATUS_IS_OK(status)) {
		d_printf("protocol negotiation failed: %s\n",
			 nt_errstr(status));
		cli_shutdown(c);
		return status;
	}

	if (smbXcli_conn_protocol(c->conn) >= PROTOCOL_SMB2_02) {
		/* Ensure we ask for some initial credits. */
		smb2cli_conn_set_max_credits(c->conn, DEFAULT_SMB2_MAX_CREDITS);
	}

	username = get_cmdline_auth_info_username(auth_info);
	password = get_cmdline_auth_info_password(auth_info);
	domain = get_cmdline_auth_info_domain(auth_info);
	if ((domain == NULL) || (domain[0] == '\0')) {
		domain = lp_workgroup();
	}

	status = cli_session_setup(c, username,
				   password, strlen(password),
				   password, strlen(password),
				   domain);
	if (!NT_STATUS_IS_OK(status)) {
		/* If a password was not supplied then
		 * try again with a null username. */
		if (password[0] || !username[0] ||
			get_cmdline_auth_info_use_kerberos(auth_info) ||
			!NT_STATUS_IS_OK(status = cli_session_setup(c, "",
				    		"", 0,
						"", 0,
					       lp_workgroup()))) {
			d_printf("session setup failed: %s\n",
				 nt_errstr(status));
			if (NT_STATUS_EQUAL(status,
					    NT_STATUS_MORE_PROCESSING_REQUIRED))
				d_printf("did you forget to run kinit?\n");
			cli_shutdown(c);
			return status;
		}
		d_printf("Anonymous login successful\n");
	}

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(10,("cli_init_creds() failed: %s\n", nt_errstr(status)));
		cli_shutdown(c);
		return status;
	}

	if ( show_sessetup ) {
		if (*c->server_domain) {
			DEBUG(0,("Domain=[%s] OS=[%s] Server=[%s]\n",
				c->server_domain,c->server_os,c->server_type));
		} else if (*c->server_os || *c->server_type) {
			DEBUG(0,("OS=[%s] Server=[%s]\n",
				 c->server_os,c->server_type));
		}
	}
	DEBUG(4,(" session setup ok\n"));

	/* here's the fun part....to support 'msdfs proxy' shares
	   (on Samba or windows) we have to issues a TRANS_GET_DFS_REFERRAL
	   here before trying to connect to the original share.
	   cli_check_msdfs_proxy() will fail if it is a normal share. */

	if (smbXcli_conn_dfs_supported(c->conn) &&
			cli_check_msdfs_proxy(ctx, c, sharename,
				&newserver, &newshare,
				force_encrypt,
				username,
				password,
				domain)) {
		cli_shutdown(c);
		return do_connect(ctx, newserver,
				newshare, auth_info, false,
				force_encrypt, max_protocol,
				port, name_type, pcli);
	}

	/* must be a normal share */

	status = cli_tree_connect(c, sharename, "?????",
				  password, strlen(password)+1);
	if (!NT_STATUS_IS_OK(status)) {
		d_printf("tree connect failed: %s\n", nt_errstr(status));
		cli_shutdown(c);
		return status;
	}

	if (force_encrypt) {
		status = cli_cm_force_encryption(c,
					username,
					password,
					domain,
					sharename);
		if (!NT_STATUS_IS_OK(status)) {
			cli_shutdown(c);
			return status;
		}
	}

	DEBUG(4,(" tconx ok\n"));
	*pcli = c;
	return NT_STATUS_OK;
}