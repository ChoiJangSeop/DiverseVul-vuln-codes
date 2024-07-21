BaseBackup(void)
{
	PGresult   *res;
	char	   *sysidentifier;
	uint32		latesttli;
	uint32		starttli;
	char		current_path[MAXPGPATH];
	char		escaped_label[MAXPGPATH];
	int			i;
	char		xlogstart[64];
	char		xlogend[64];
	int			minServerMajor,
				maxServerMajor;
	int			serverMajor;

	/*
	 * Connect in replication mode to the server
	 */
	conn = GetConnection();
	if (!conn)
		/* Error message already written in GetConnection() */
		exit(1);

	/*
	 * Check server version. BASE_BACKUP command was introduced in 9.1, so we
	 * can't work with servers older than 9.1.
	 */
	minServerMajor = 901;
	maxServerMajor = PG_VERSION_NUM / 100;
	serverMajor = PQserverVersion(conn) / 100;
	if (serverMajor < minServerMajor || serverMajor > maxServerMajor)
	{
		const char *serverver = PQparameterStatus(conn, "server_version");

		fprintf(stderr, _("%s: incompatible server version %s\n"),
				progname, serverver ? serverver : "'unknown'");
		disconnect_and_exit(1);
	}

	/*
	 * If WAL streaming was requested, also check that the server is new
	 * enough for that.
	 */
	if (streamwal && !CheckServerVersionForStreaming(conn))
	{
		/* Error message already written in CheckServerVersionForStreaming() */
		disconnect_and_exit(1);
	}

	/*
	 * Build contents of recovery.conf if requested
	 */
	if (writerecoveryconf)
		GenerateRecoveryConf(conn);

	/*
	 * Run IDENTIFY_SYSTEM so we can get the timeline
	 */
	res = PQexec(conn, "IDENTIFY_SYSTEM");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, _("%s: could not send replication command \"%s\": %s"),
				progname, "IDENTIFY_SYSTEM", PQerrorMessage(conn));
		disconnect_and_exit(1);
	}
	if (PQntuples(res) != 1 || PQnfields(res) != 3)
	{
		fprintf(stderr,
				_("%s: could not identify system: got %d rows and %d fields, expected %d rows and %d fields\n"),
				progname, PQntuples(res), PQnfields(res), 1, 3);
		disconnect_and_exit(1);
	}
	sysidentifier = pg_strdup(PQgetvalue(res, 0, 0));
	latesttli = atoi(PQgetvalue(res, 0, 1));
	PQclear(res);

	/*
	 * Start the actual backup
	 */
	PQescapeStringConn(conn, escaped_label, label, sizeof(escaped_label), &i);
	snprintf(current_path, sizeof(current_path),
			 "BASE_BACKUP LABEL '%s' %s %s %s %s",
			 escaped_label,
			 showprogress ? "PROGRESS" : "",
			 includewal && !streamwal ? "WAL" : "",
			 fastcheckpoint ? "FAST" : "",
			 includewal ? "NOWAIT" : "");

	if (PQsendQuery(conn, current_path) == 0)
	{
		fprintf(stderr, _("%s: could not send replication command \"%s\": %s"),
				progname, "BASE_BACKUP", PQerrorMessage(conn));
		disconnect_and_exit(1);
	}

	/*
	 * Get the starting xlog position
	 */
	res = PQgetResult(conn);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, _("%s: could not initiate base backup: %s"),
				progname, PQerrorMessage(conn));
		disconnect_and_exit(1);
	}
	if (PQntuples(res) != 1)
	{
		fprintf(stderr,
				_("%s: server returned unexpected response to BASE_BACKUP command; got %d rows and %d fields, expected %d rows and %d fields\n"),
				progname, PQntuples(res), PQnfields(res), 1, 2);
		disconnect_and_exit(1);
	}

	strcpy(xlogstart, PQgetvalue(res, 0, 0));

	/*
	 * 9.3 and later sends the TLI of the starting point. With older servers,
	 * assume it's the same as the latest timeline reported by
	 * IDENTIFY_SYSTEM.
	 */
	if (PQnfields(res) >= 2)
		starttli = atoi(PQgetvalue(res, 0, 1));
	else
		starttli = latesttli;
	PQclear(res);
	MemSet(xlogend, 0, sizeof(xlogend));

	if (verbose && includewal)
		fprintf(stderr, _("transaction log start point: %s on timeline %u\n"),
				xlogstart, starttli);

	/*
	 * Get the header
	 */
	res = PQgetResult(conn);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, _("%s: could not get backup header: %s"),
				progname, PQerrorMessage(conn));
		disconnect_and_exit(1);
	}
	if (PQntuples(res) < 1)
	{
		fprintf(stderr, _("%s: no data returned from server\n"), progname);
		disconnect_and_exit(1);
	}

	/*
	 * Sum up the total size, for progress reporting
	 */
	totalsize = totaldone = 0;
	tablespacecount = PQntuples(res);
	for (i = 0; i < PQntuples(res); i++)
	{
		totalsize += atol(PQgetvalue(res, i, 2));

		/*
		 * Verify tablespace directories are empty. Don't bother with the
		 * first once since it can be relocated, and it will be checked before
		 * we do anything anyway.
		 */
		if (format == 'p' && !PQgetisnull(res, i, 1))
			verify_dir_is_empty_or_create(PQgetvalue(res, i, 1));
	}

	/*
	 * When writing to stdout, require a single tablespace
	 */
	if (format == 't' && strcmp(basedir, "-") == 0 && PQntuples(res) > 1)
	{
		fprintf(stderr,
				_("%s: can only write single tablespace to stdout, database has %d\n"),
				progname, PQntuples(res));
		disconnect_and_exit(1);
	}

	/*
	 * If we're streaming WAL, start the streaming session before we start
	 * receiving the actual data chunks.
	 */
	if (streamwal)
	{
		if (verbose)
			fprintf(stderr, _("%s: starting background WAL receiver\n"),
					progname);
		StartLogStreamer(xlogstart, starttli, sysidentifier);
	}

	/*
	 * Start receiving chunks
	 */
	for (i = 0; i < PQntuples(res); i++)
	{
		if (format == 't')
			ReceiveTarFile(conn, res, i);
		else
			ReceiveAndUnpackTarFile(conn, res, i);
	}							/* Loop over all tablespaces */

	if (showprogress)
	{
		progress_report(PQntuples(res), NULL, true);
		fprintf(stderr, "\n");	/* Need to move to next line */
	}
	PQclear(res);

	/*
	 * Get the stop position
	 */
	res = PQgetResult(conn);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr,
		 _("%s: could not get transaction log end position from server: %s"),
				progname, PQerrorMessage(conn));
		disconnect_and_exit(1);
	}
	if (PQntuples(res) != 1)
	{
		fprintf(stderr,
			 _("%s: no transaction log end position returned from server\n"),
				progname);
		disconnect_and_exit(1);
	}
	strcpy(xlogend, PQgetvalue(res, 0, 0));
	if (verbose && includewal)
		fprintf(stderr, "transaction log end point: %s\n", xlogend);
	PQclear(res);

	res = PQgetResult(conn);
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
		fprintf(stderr, _("%s: final receive failed: %s"),
				progname, PQerrorMessage(conn));
		disconnect_and_exit(1);
	}

	if (bgchild > 0)
	{
#ifndef WIN32
		int			status;
		int			r;
#else
		DWORD		status;
		uint32		hi,
					lo;
#endif

		if (verbose)
			fprintf(stderr,
					_("%s: waiting for background process to finish streaming ...\n"), progname);

#ifndef WIN32
		if (write(bgpipe[1], xlogend, strlen(xlogend)) != strlen(xlogend))
		{
			fprintf(stderr,
					_("%s: could not send command to background pipe: %s\n"),
					progname, strerror(errno));
			disconnect_and_exit(1);
		}

		/* Just wait for the background process to exit */
		r = waitpid(bgchild, &status, 0);
		if (r == -1)
		{
			fprintf(stderr, _("%s: could not wait for child process: %s\n"),
					progname, strerror(errno));
			disconnect_and_exit(1);
		}
		if (r != bgchild)
		{
			fprintf(stderr, _("%s: child %d died, expected %d\n"),
					progname, r, (int) bgchild);
			disconnect_and_exit(1);
		}
		if (!WIFEXITED(status))
		{
			fprintf(stderr, _("%s: child process did not exit normally\n"),
					progname);
			disconnect_and_exit(1);
		}
		if (WEXITSTATUS(status) != 0)
		{
			fprintf(stderr, _("%s: child process exited with error %d\n"),
					progname, WEXITSTATUS(status));
			disconnect_and_exit(1);
		}
		/* Exited normally, we're happy! */
#else							/* WIN32 */

		/*
		 * On Windows, since we are in the same process, we can just store the
		 * value directly in the variable, and then set the flag that says
		 * it's there.
		 */
		if (sscanf(xlogend, "%X/%X", &hi, &lo) != 2)
		{
			fprintf(stderr,
				  _("%s: could not parse transaction log location \"%s\"\n"),
					progname, xlogend);
			disconnect_and_exit(1);
		}
		xlogendptr = ((uint64) hi) << 32 | lo;
		InterlockedIncrement(&has_xlogendptr);

		/* First wait for the thread to exit */
		if (WaitForSingleObjectEx((HANDLE) bgchild, INFINITE, FALSE) !=
			WAIT_OBJECT_0)
		{
			_dosmaperr(GetLastError());
			fprintf(stderr, _("%s: could not wait for child thread: %s\n"),
					progname, strerror(errno));
			disconnect_and_exit(1);
		}
		if (GetExitCodeThread((HANDLE) bgchild, &status) == 0)
		{
			_dosmaperr(GetLastError());
			fprintf(stderr, _("%s: could not get child thread exit status: %s\n"),
					progname, strerror(errno));
			disconnect_and_exit(1);
		}
		if (status != 0)
		{
			fprintf(stderr, _("%s: child thread exited with error %u\n"),
					progname, (unsigned int) status);
			disconnect_and_exit(1);
		}
		/* Exited normally, we're happy */
#endif
	}

	/* Free the recovery.conf contents */
	destroyPQExpBuffer(recoveryconfcontents);

	/*
	 * End of copy data. Final result is already checked inside the loop.
	 */
	PQclear(res);
	PQfinish(conn);

	if (verbose)
		fprintf(stderr, "%s: base backup completed\n", progname);
}