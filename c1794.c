ReceiveAndUnpackTarFile(PGconn *conn, PGresult *res, int rownum)
{
	char		current_path[MAXPGPATH];
	char		filename[MAXPGPATH];
	int			current_len_left;
	int			current_padding = 0;
	bool		basetablespace = PQgetisnull(res, rownum, 0);
	char	   *copybuf = NULL;
	FILE	   *file = NULL;

	if (basetablespace)
		strcpy(current_path, basedir);
	else
		strcpy(current_path, PQgetvalue(res, rownum, 1));

	/*
	 * Get the COPY data
	 */
	res = PQgetResult(conn);
	if (PQresultStatus(res) != PGRES_COPY_OUT)
	{
		fprintf(stderr, _("%s: could not get COPY data stream: %s"),
				progname, PQerrorMessage(conn));
		disconnect_and_exit(1);
	}

	while (1)
	{
		int			r;

		if (copybuf != NULL)
		{
			PQfreemem(copybuf);
			copybuf = NULL;
		}

		r = PQgetCopyData(conn, &copybuf, 0);

		if (r == -1)
		{
			/*
			 * End of chunk
			 */
			if (file)
				fclose(file);

			break;
		}
		else if (r == -2)
		{
			fprintf(stderr, _("%s: could not read COPY data: %s"),
					progname, PQerrorMessage(conn));
			disconnect_and_exit(1);
		}

		if (file == NULL)
		{
			int			filemode;

			/*
			 * No current file, so this must be the header for a new file
			 */
			if (r != 512)
			{
				fprintf(stderr, _("%s: invalid tar block header size: %d\n"),
						progname, r);
				disconnect_and_exit(1);
			}
			totaldone += 512;

			if (sscanf(copybuf + 124, "%11o", &current_len_left) != 1)
			{
				fprintf(stderr, _("%s: could not parse file size\n"),
						progname);
				disconnect_and_exit(1);
			}

			/* Set permissions on the file */
			if (sscanf(&copybuf[100], "%07o ", &filemode) != 1)
			{
				fprintf(stderr, _("%s: could not parse file mode\n"),
						progname);
				disconnect_and_exit(1);
			}

			/*
			 * All files are padded up to 512 bytes
			 */
			current_padding =
				((current_len_left + 511) & ~511) - current_len_left;

			/*
			 * First part of header is zero terminated filename
			 */
			snprintf(filename, sizeof(filename), "%s/%s", current_path,
					 copybuf);
			if (filename[strlen(filename) - 1] == '/')
			{
				/*
				 * Ends in a slash means directory or symlink to directory
				 */
				if (copybuf[156] == '5')
				{
					/*
					 * Directory
					 */
					filename[strlen(filename) - 1] = '\0';		/* Remove trailing slash */
					if (mkdir(filename, S_IRWXU) != 0)
					{
						/*
						 * When streaming WAL, pg_xlog will have been created
						 * by the wal receiver process. Also, when transaction
						 * log directory location was specified, pg_xlog has
						 * already been created as a symbolic link before
						 * starting the actual backup. So just ignore failure
						 * on them.
						 */
						if ((!streamwal && (strcmp(xlog_dir, "") == 0))
							|| strcmp(filename + strlen(filename) - 8, "/pg_xlog") != 0)
						{
							fprintf(stderr,
							_("%s: could not create directory \"%s\": %s\n"),
									progname, filename, strerror(errno));
							disconnect_and_exit(1);
						}
					}
#ifndef WIN32
					if (chmod(filename, (mode_t) filemode))
						fprintf(stderr,
								_("%s: could not set permissions on directory \"%s\": %s\n"),
								progname, filename, strerror(errno));
#endif
				}
				else if (copybuf[156] == '2')
				{
					/*
					 * Symbolic link
					 */
					filename[strlen(filename) - 1] = '\0';		/* Remove trailing slash */
					if (symlink(&copybuf[157], filename) != 0)
					{
						fprintf(stderr,
								_("%s: could not create symbolic link from \"%s\" to \"%s\": %s\n"),
						 progname, filename, &copybuf[157], strerror(errno));
						disconnect_and_exit(1);
					}
				}
				else
				{
					fprintf(stderr,
							_("%s: unrecognized link indicator \"%c\"\n"),
							progname, copybuf[156]);
					disconnect_and_exit(1);
				}
				continue;		/* directory or link handled */
			}

			/*
			 * regular file
			 */
			file = fopen(filename, "wb");
			if (!file)
			{
				fprintf(stderr, _("%s: could not create file \"%s\": %s\n"),
						progname, filename, strerror(errno));
				disconnect_and_exit(1);
			}

#ifndef WIN32
			if (chmod(filename, (mode_t) filemode))
				fprintf(stderr, _("%s: could not set permissions on file \"%s\": %s\n"),
						progname, filename, strerror(errno));
#endif

			if (current_len_left == 0)
			{
				/*
				 * Done with this file, next one will be a new tar header
				 */
				fclose(file);
				file = NULL;
				continue;
			}
		}						/* new file */
		else
		{
			/*
			 * Continuing blocks in existing file
			 */
			if (current_len_left == 0 && r == current_padding)
			{
				/*
				 * Received the padding block for this file, ignore it and
				 * close the file, then move on to the next tar header.
				 */
				fclose(file);
				file = NULL;
				totaldone += r;
				continue;
			}

			if (fwrite(copybuf, r, 1, file) != 1)
			{
				fprintf(stderr, _("%s: could not write to file \"%s\": %s\n"),
						progname, filename, strerror(errno));
				disconnect_and_exit(1);
			}
			totaldone += r;
			progress_report(rownum, filename, false);

			current_len_left -= r;
			if (current_len_left == 0 && current_padding == 0)
			{
				/*
				 * Received the last block, and there is no padding to be
				 * expected. Close the file and move on to the next tar
				 * header.
				 */
				fclose(file);
				file = NULL;
				continue;
			}
		}						/* continuing data in existing file */
	}							/* loop over all data blocks */
	progress_report(rownum, filename, true);

	if (file != NULL)
	{
		fprintf(stderr,
				_("%s: COPY stream ended before last file was finished\n"),
				progname);
		disconnect_and_exit(1);
	}

	if (copybuf != NULL)
		PQfreemem(copybuf);

	if (basetablespace && writerecoveryconf)
		WriteRecoveryConf();
}