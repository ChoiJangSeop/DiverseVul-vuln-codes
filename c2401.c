pg_GSS_recvauth(Port *port)
{
	OM_uint32	maj_stat,
				min_stat,
				lmin_s,
				gflags;
	int			mtype;
	int			ret;
	StringInfoData buf;
	gss_buffer_desc gbuf;

	/*
	 * GSS auth is not supported for protocol versions before 3, because it
	 * relies on the overall message length word to determine the GSS payload
	 * size in AuthenticationGSSContinue and PasswordMessage messages. (This
	 * is, in fact, a design error in our GSS support, because protocol
	 * messages are supposed to be parsable without relying on the length
	 * word; but it's not worth changing it now.)
	 */
	if (PG_PROTOCOL_MAJOR(FrontendProtocol) < 3)
		ereport(FATAL,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("GSSAPI is not supported in protocol version 2")));

	if (pg_krb_server_keyfile && strlen(pg_krb_server_keyfile) > 0)
	{
		/*
		 * Set default Kerberos keytab file for the Krb5 mechanism.
		 *
		 * setenv("KRB5_KTNAME", pg_krb_server_keyfile, 0); except setenv()
		 * not always available.
		 */
		if (getenv("KRB5_KTNAME") == NULL)
		{
			size_t		kt_len = strlen(pg_krb_server_keyfile) + 14;
			char	   *kt_path = malloc(kt_len);

			if (!kt_path)
			{
				ereport(LOG,
						(errcode(ERRCODE_OUT_OF_MEMORY),
						 errmsg("out of memory")));
				return STATUS_ERROR;
			}
			snprintf(kt_path, kt_len, "KRB5_KTNAME=%s", pg_krb_server_keyfile);
			putenv(kt_path);
		}
	}

	/*
	 * We accept any service principal that's present in our keytab. This
	 * increases interoperability between kerberos implementations that see
	 * for example case sensitivity differently, while not really opening up
	 * any vector of attack.
	 */
	port->gss->cred = GSS_C_NO_CREDENTIAL;

	/*
	 * Initialize sequence with an empty context
	 */
	port->gss->ctx = GSS_C_NO_CONTEXT;

	/*
	 * Loop through GSSAPI message exchange. This exchange can consist of
	 * multiple messags sent in both directions. First message is always from
	 * the client. All messages from client to server are password packets
	 * (type 'p').
	 */
	do
	{
		mtype = pq_getbyte();
		if (mtype != 'p')
		{
			/* Only log error if client didn't disconnect. */
			if (mtype != EOF)
				ereport(COMMERROR,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg("expected GSS response, got message type %d",
								mtype)));
			return STATUS_ERROR;
		}

		/* Get the actual GSS token */
		initStringInfo(&buf);
		if (pq_getmessage(&buf, PG_MAX_AUTH_TOKEN_LENGTH))
		{
			/* EOF - pq_getmessage already logged error */
			pfree(buf.data);
			return STATUS_ERROR;
		}

		/* Map to GSSAPI style buffer */
		gbuf.length = buf.len;
		gbuf.value = buf.data;

		elog(DEBUG4, "Processing received GSS token of length %u",
			 (unsigned int) gbuf.length);

		maj_stat = gss_accept_sec_context(
										  &min_stat,
										  &port->gss->ctx,
										  port->gss->cred,
										  &gbuf,
										  GSS_C_NO_CHANNEL_BINDINGS,
										  &port->gss->name,
										  NULL,
										  &port->gss->outbuf,
										  &gflags,
										  NULL,
										  NULL);

		/* gbuf no longer used */
		pfree(buf.data);

		elog(DEBUG5, "gss_accept_sec_context major: %d, "
			 "minor: %d, outlen: %u, outflags: %x",
			 maj_stat, min_stat,
			 (unsigned int) port->gss->outbuf.length, gflags);

		if (port->gss->outbuf.length != 0)
		{
			/*
			 * Negotiation generated data to be sent to the client.
			 */
			elog(DEBUG4, "sending GSS response token of length %u",
				 (unsigned int) port->gss->outbuf.length);

			sendAuthRequest(port, AUTH_REQ_GSS_CONT);

			gss_release_buffer(&lmin_s, &port->gss->outbuf);
		}

		if (maj_stat != GSS_S_COMPLETE && maj_stat != GSS_S_CONTINUE_NEEDED)
		{
			gss_delete_sec_context(&lmin_s, &port->gss->ctx, GSS_C_NO_BUFFER);
			pg_GSS_error(ERROR,
					   gettext_noop("accepting GSS security context failed"),
						 maj_stat, min_stat);
		}

		if (maj_stat == GSS_S_CONTINUE_NEEDED)
			elog(DEBUG4, "GSS continue needed");

	} while (maj_stat == GSS_S_CONTINUE_NEEDED);

	if (port->gss->cred != GSS_C_NO_CREDENTIAL)
	{
		/*
		 * Release service principal credentials
		 */
		gss_release_cred(&min_stat, &port->gss->cred);
	}

	/*
	 * GSS_S_COMPLETE indicates that authentication is now complete.
	 *
	 * Get the name of the user that authenticated, and compare it to the pg
	 * username that was specified for the connection.
	 */
	maj_stat = gss_display_name(&min_stat, port->gss->name, &gbuf, NULL);
	if (maj_stat != GSS_S_COMPLETE)
		pg_GSS_error(ERROR,
					 gettext_noop("retrieving GSS user name failed"),
					 maj_stat, min_stat);

	/*
	 * Split the username at the realm separator
	 */
	if (strchr(gbuf.value, '@'))
	{
		char	   *cp = strchr(gbuf.value, '@');

		/*
		 * If we are not going to include the realm in the username that is
		 * passed to the ident map, destructively modify it here to remove the
		 * realm. Then advance past the separator to check the realm.
		 */
		if (!port->hba->include_realm)
			*cp = '\0';
		cp++;

		if (port->hba->krb_realm != NULL && strlen(port->hba->krb_realm))
		{
			/*
			 * Match the realm part of the name first
			 */
			if (pg_krb_caseins_users)
				ret = pg_strcasecmp(port->hba->krb_realm, cp);
			else
				ret = strcmp(port->hba->krb_realm, cp);

			if (ret)
			{
				/* GSS realm does not match */
				elog(DEBUG2,
				   "GSSAPI realm (%s) and configured realm (%s) don't match",
					 cp, port->hba->krb_realm);
				gss_release_buffer(&lmin_s, &gbuf);
				return STATUS_ERROR;
			}
		}
	}
	else if (port->hba->krb_realm && strlen(port->hba->krb_realm))
	{
		elog(DEBUG2,
			 "GSSAPI did not return realm but realm matching was requested");

		gss_release_buffer(&lmin_s, &gbuf);
		return STATUS_ERROR;
	}

	ret = check_usermap(port->hba->usermap, port->user_name, gbuf.value,
						pg_krb_caseins_users);

	gss_release_buffer(&lmin_s, &gbuf);

	return ret;
}