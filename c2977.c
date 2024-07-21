int setpwnam(struct passwd *pwd)
{
	FILE *fp = NULL, *pwf = NULL;
	int save_errno;
	int found;
	int namelen;
	int buflen = 256;
	int contlen, rc;
	char *linebuf = NULL;
	char *tmpname = NULL;
	char *atomic_dir = "/etc";

	pw_init();

	if ((fp = xfmkstemp(&tmpname, atomic_dir)) == NULL)
		return -1;

	/* ptmp should be owned by root.root or root.wheel */
	if (fchown(fileno(fp), (uid_t) 0, (gid_t) 0) < 0)
		goto fail;

	/* acquire exclusive lock */
	if (lckpwdf() < 0)
		goto fail;
	pwf = fopen(PASSWD_FILE, "r");
	if (!pwf)
		goto fail;

	namelen = strlen(pwd->pw_name);

	linebuf = malloc(buflen);
	if (!linebuf)
		goto fail;

	/* parse the passwd file */
	found = false;

	/* Do you wonder why I don't use getpwent? Read comments at top of
	 * file */
	while (fgets(linebuf, buflen, pwf) != NULL) {
		contlen = strlen(linebuf);
		while (linebuf[contlen - 1] != '\n' && !feof(pwf)) {
			char *tmp;
			/* Extend input buffer if it failed getting the whole line,
			 * so now we double the buffer size */
			buflen *= 2;
			tmp = realloc(linebuf, buflen);
			if (tmp == NULL)
				goto fail;
			linebuf = tmp;
			/* And fill the rest of the buffer */
			if (fgets(&linebuf[contlen], buflen / 2, pwf) == NULL)
				break;
			contlen = strlen(linebuf);
			/* That was a lot of work for nothing. Gimme perl! */
		}

		/* Is this the username we were sent to change? */
		if (!found && linebuf[namelen] == ':' &&
		    !strncmp(linebuf, pwd->pw_name, namelen)) {
			/* Yes! So go forth in the name of the Lord and
			 * change it!  */
			if (putpwent(pwd, fp) < 0)
				goto fail;
			found = true;
			continue;
		}
		/* Nothing in particular happened, copy input to output */
		fputs(linebuf, fp);
	}

	/* xfmkstemp is too restrictive by default for passwd file */
	if (fchmod(fileno(fp), 0644) < 0)
		goto fail;
	rc = close_stream(fp);
	fp = NULL;
	if (rc != 0)
		goto fail;

	fclose(pwf);	/* I don't think I want to know if this failed */
	pwf = NULL;

	if (!found) {
		errno = ENOENT;	/* give me something better */
		goto fail;
	}

	/* we don't care if we can't remove the backup file */
	unlink(PASSWD_FILE ".OLD");
	/* we don't care if we can't create the backup file */
	ignore_result(link(PASSWD_FILE, PASSWD_FILE ".OLD"));
	/* we DO care if we can't rename to the passwd file */
	if (rename(tmpname, PASSWD_FILE) < 0)
		goto fail;
	/* finally:  success */
	ulckpwdf();
	return 0;

 fail:
	save_errno = errno;
	ulckpwdf();
	if (fp != NULL)
		fclose(fp);
	if (tmpname != NULL)
		unlink(tmpname);
	free(tmpname);
	if (pwf != NULL)
		fclose(pwf);
	free(linebuf);
	errno = save_errno;
	return -1;
}