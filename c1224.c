static void make_device(char *device_name, char *path, int operation)
{
	int major, minor, type, len;

	if (G.verbose)
		bb_error_msg("device: %s, %s", device_name, path);

	/* Try to read major/minor string.  Note that the kernel puts \n after
	 * the data, so we don't need to worry about null terminating the string
	 * because sscanf() will stop at the first nondigit, which \n is.
	 * We also depend on path having writeable space after it.
	 */
	major = -1;
	if (operation == OP_add) {
		char *dev_maj_min = path + strlen(path);

		strcpy(dev_maj_min, "/dev");
		len = open_read_close(path, dev_maj_min + 1, 64);
		*dev_maj_min = '\0';
		if (len < 1) {
			if (!ENABLE_FEATURE_MDEV_EXEC)
				return;
			/* no "dev" file, but we can still run scripts
			 * based on device name */
		} else if (sscanf(++dev_maj_min, "%u:%u", &major, &minor) == 2) {
			if (G.verbose)
				bb_error_msg("maj,min: %u,%u", major, minor);
		} else {
			major = -1;
		}
	}
	/* else: for delete, -1 still deletes the node, but < -1 suppresses that */

	/* Determine device name, type, major and minor */
	if (!device_name)
		device_name = (char*) bb_basename(path);
	/* http://kernel.org/doc/pending/hotplug.txt says that only
	 * "/sys/block/..." is for block devices. "/sys/bus" etc is not.
	 * But since 2.6.25 block devices are also in /sys/class/block.
	 * We use strstr("/block/") to forestall future surprises.
	 */
	type = S_IFCHR;
	if (strstr(path, "/block/") || (G.subsystem && strncmp(G.subsystem, "block", 5) == 0))
		type = S_IFBLK;

#if ENABLE_FEATURE_MDEV_CONF
	G.rule_idx = 0; /* restart from the beginning (think mdev -s) */
#endif
	for (;;) {
		const char *str_to_match;
		regmatch_t off[1 + 9 * ENABLE_FEATURE_MDEV_RENAME_REGEXP];
		char *command;
		char *alias;
		char aliaslink = aliaslink; /* for compiler */
		char *node_name;
		const struct rule *rule;

		str_to_match = device_name;

		rule = next_rule();

#if ENABLE_FEATURE_MDEV_CONF
		if (rule->maj >= 0) {  /* @maj,min rule */
			if (major != rule->maj)
				continue;
			if (minor < rule->min0 || minor > rule->min1)
				continue;
			memset(off, 0, sizeof(off));
			goto rule_matches;
		}
		if (rule->envvar) { /* $envvar=regex rule */
			str_to_match = getenv(rule->envvar);
			dbg("getenv('%s'):'%s'", rule->envvar, str_to_match);
			if (!str_to_match)
				continue;
		}
		/* else: str_to_match = device_name */

		if (rule->regex_compiled) {
			int regex_match = regexec(&rule->match, str_to_match, ARRAY_SIZE(off), off, 0);
			dbg("regex_match for '%s':%d", str_to_match, regex_match);
			//bb_error_msg("matches:");
			//for (int i = 0; i < ARRAY_SIZE(off); i++) {
			//	if (off[i].rm_so < 0) continue;
			//	bb_error_msg("match %d: '%.*s'\n", i,
			//		(int)(off[i].rm_eo - off[i].rm_so),
			//		device_name + off[i].rm_so);
			//}

			if (regex_match != 0
			/* regexec returns whole pattern as "range" 0 */
			 || off[0].rm_so != 0
			 || (int)off[0].rm_eo != (int)strlen(str_to_match)
			) {
				continue; /* this rule doesn't match */
			}
		}
		/* else: it's final implicit "match-all" rule */
 rule_matches:
#endif
		dbg("rule matched");

		/* Build alias name */
		alias = NULL;
		if (ENABLE_FEATURE_MDEV_RENAME && rule->ren_mov) {
			aliaslink = rule->ren_mov[0];
			if (aliaslink == '!') {
				/* "!": suppress node creation/deletion */
				major = -2;
			}
			else if (aliaslink == '>' || aliaslink == '=') {
				if (ENABLE_FEATURE_MDEV_RENAME_REGEXP) {
					char *s;
					char *p;
					unsigned n;

					/* substitute %1..9 with off[1..9], if any */
					n = 0;
					s = rule->ren_mov;
					while (*s)
						if (*s++ == '%')
							n++;

					p = alias = xzalloc(strlen(rule->ren_mov) + n * strlen(str_to_match));
					s = rule->ren_mov + 1;
					while (*s) {
						*p = *s;
						if ('%' == *s) {
							unsigned i = (s[1] - '0');
							if (i <= 9 && off[i].rm_so >= 0) {
								n = off[i].rm_eo - off[i].rm_so;
								strncpy(p, str_to_match + off[i].rm_so, n);
								p += n - 1;
								s++;
							}
						}
						p++;
						s++;
					}
				} else {
					alias = xstrdup(rule->ren_mov + 1);
				}
			}
		}
		dbg("alias:'%s'", alias);

		command = NULL;
		IF_FEATURE_MDEV_EXEC(command = rule->r_cmd;)
		if (command) {
			const char *s = "$@*";
			const char *s2 = strchr(s, command[0]);

			/* Are we running this command now?
			 * Run $cmd on delete, @cmd on create, *cmd on both
			 */
			if (s2 - s != (operation == OP_remove) || *s2 == '*') {
				/* We are here if: '*',
				 * or: '@' and delete = 0,
				 * or: '$' and delete = 1
				 */
				command++;
			} else {
				command = NULL;
			}
		}
		dbg("command:'%s'", command);

		/* "Execute" the line we found */
		node_name = device_name;
		if (ENABLE_FEATURE_MDEV_RENAME && alias) {
			node_name = alias = build_alias(alias, device_name);
			dbg("alias2:'%s'", alias);
		}

		if (operation == OP_add && major >= 0) {
			char *slash = strrchr(node_name, '/');
			if (slash) {
				*slash = '\0';
				bb_make_directory(node_name, 0755, FILEUTILS_RECUR);
				*slash = '/';
			}
			if (G.verbose)
				bb_error_msg("mknod: %s (%d,%d) %o", node_name, major, minor, rule->mode | type);
			if (mknod(node_name, rule->mode | type, makedev(major, minor)) && errno != EEXIST)
				bb_perror_msg("can't create '%s'", node_name);
			if (ENABLE_FEATURE_MDEV_CONF) {
				chmod(node_name, rule->mode);
				chown(node_name, rule->ugid.uid, rule->ugid.gid);
			}
			if (major == G.root_major && minor == G.root_minor)
				symlink(node_name, "root");
			if (ENABLE_FEATURE_MDEV_RENAME && alias) {
				if (aliaslink == '>') {
//TODO: on devtmpfs, device_name already exists and symlink() fails.
//End result is that instead of symlink, we have two nodes.
//What should be done?
					if (G.verbose)
						bb_error_msg("symlink: %s", device_name);
					symlink(node_name, device_name);
				}
			}
		}

		if (ENABLE_FEATURE_MDEV_EXEC && command) {
			/* setenv will leak memory, use putenv/unsetenv/free */
			char *s = xasprintf("%s=%s", "MDEV", node_name);
			char *s1 = xasprintf("%s=%s", "SUBSYSTEM", G.subsystem);
			putenv(s);
			putenv(s1);
			if (G.verbose)
				bb_error_msg("running: %s", command);
			if (system(command) == -1)
				bb_perror_msg("can't run '%s'", command);
			bb_unsetenv_and_free(s1);
			bb_unsetenv_and_free(s);
		}

		if (operation == OP_remove && major >= -1) {
			if (ENABLE_FEATURE_MDEV_RENAME && alias) {
				if (aliaslink == '>') {
					if (G.verbose)
						bb_error_msg("unlink: %s", device_name);
					unlink(device_name);
				}
			}
			if (G.verbose)
				bb_error_msg("unlink: %s", node_name);
			unlink(node_name);
		}

		if (ENABLE_FEATURE_MDEV_RENAME)
			free(alias);

		/* We found matching line.
		 * Stop unless it was prefixed with '-'
		 */
		if (!ENABLE_FEATURE_MDEV_CONF || !rule->keep_matching)
			break;
	} /* for (;;) */
}