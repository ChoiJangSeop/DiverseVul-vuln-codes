assemble_mountinfo(struct parsed_mount_info *parsed_info,
		   const char *thisprogram, const char *mountpoint,
		   const char *orig_dev, char *orgoptions)
{
	int rc;

	rc = drop_child_privs();
	if (rc)
		goto assemble_exit;

	if (getuid()) {
		rc = check_fstab(thisprogram, mountpoint, orig_dev,
				 &orgoptions);
		if (rc)
			goto assemble_exit;

		/* enable any default user mount flags */
		parsed_info->flags |= CIFS_SETUID_FLAGS;
	}

	rc = get_pw_from_env(parsed_info);
	if (rc)
		goto assemble_exit;

	if (orgoptions) {
		rc = parse_options(orgoptions, parsed_info);
		if (rc)
			goto assemble_exit;
	}

	if (getuid()) {
		if (!(parsed_info->flags & (MS_USERS | MS_USER))) {
			fprintf(stderr, "%s: permission denied\n", thisprogram);
			rc = EX_USAGE;
			goto assemble_exit;
		}
	}

	parsed_info->flags &= ~(MS_USERS | MS_USER);

	rc = parse_unc(orig_dev, parsed_info);
	if (rc)
		goto assemble_exit;

	rc = resolve_host(parsed_info);
	if (rc)
		goto assemble_exit;

	if (!parsed_info->got_user) {
		/*
		 * Note that the password will not be retrieved from the
		 * USER env variable (ie user%password form) as there is
		 * already a PASSWD environment varaible
		 */
		if (getenv("USER"))
			strlcpy(parsed_info->username, getenv("USER"),
				sizeof(parsed_info->username));
		else
			strlcpy(parsed_info->username, getusername(),
				sizeof(parsed_info->username));
		parsed_info->got_user = 1;
	}

	if (!parsed_info->got_password) {
		/* getpass is obsolete, but there's apparently nothing that replaces it */
		char *tmp_pass = getpass("Password: ");
		if (!tmp_pass) {
			fprintf(stderr, "Error reading password, exiting\n");
			rc = EX_SYSERR;
			goto assemble_exit;
		}
		rc = set_password(parsed_info, tmp_pass);
		if (rc)
			goto assemble_exit;
	}

	/* copy in ver= string. It's not really needed, but what the hell */
	strlcat(parsed_info->options, ",ver=", sizeof(parsed_info->options));
	strlcat(parsed_info->options, OPTIONS_VERSION, sizeof(parsed_info->options));

	/* copy in user= string */
	if (parsed_info->got_user) {
		strlcat(parsed_info->options, ",user=",
			sizeof(parsed_info->options));
		strlcat(parsed_info->options, parsed_info->username,
			sizeof(parsed_info->options));
	}

	if (*parsed_info->domain) {
		strlcat(parsed_info->options, ",domain=",
			sizeof(parsed_info->options));
		strlcat(parsed_info->options, parsed_info->domain,
			sizeof(parsed_info->options));
	}

assemble_exit:
	return rc;
}