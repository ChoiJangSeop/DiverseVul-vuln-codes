const char *setup_git_directory_gently(int *nongit_ok)
{
	static struct strbuf cwd = STRBUF_INIT;
	struct strbuf dir = STRBUF_INIT, gitdir = STRBUF_INIT;
	const char *prefix = NULL;
	struct repository_format repo_fmt = REPOSITORY_FORMAT_INIT;

	/*
	 * We may have read an incomplete configuration before
	 * setting-up the git directory. If so, clear the cache so
	 * that the next queries to the configuration reload complete
	 * configuration (including the per-repo config file that we
	 * ignored previously).
	 */
	git_config_clear();

	/*
	 * Let's assume that we are in a git repository.
	 * If it turns out later that we are somewhere else, the value will be
	 * updated accordingly.
	 */
	if (nongit_ok)
		*nongit_ok = 0;

	if (strbuf_getcwd(&cwd))
		die_errno(_("Unable to read current working directory"));
	strbuf_addbuf(&dir, &cwd);

	switch (setup_git_directory_gently_1(&dir, &gitdir, 1)) {
	case GIT_DIR_EXPLICIT:
		prefix = setup_explicit_git_dir(gitdir.buf, &cwd, &repo_fmt, nongit_ok);
		break;
	case GIT_DIR_DISCOVERED:
		if (dir.len < cwd.len && chdir(dir.buf))
			die(_("cannot change to '%s'"), dir.buf);
		prefix = setup_discovered_git_dir(gitdir.buf, &cwd, dir.len,
						  &repo_fmt, nongit_ok);
		break;
	case GIT_DIR_BARE:
		if (dir.len < cwd.len && chdir(dir.buf))
			die(_("cannot change to '%s'"), dir.buf);
		prefix = setup_bare_git_dir(&cwd, dir.len, &repo_fmt, nongit_ok);
		break;
	case GIT_DIR_HIT_CEILING:
		if (!nongit_ok)
			die(_("not a git repository (or any of the parent directories): %s"),
			    DEFAULT_GIT_DIR_ENVIRONMENT);
		*nongit_ok = 1;
		break;
	case GIT_DIR_HIT_MOUNT_POINT:
		if (!nongit_ok)
			die(_("not a git repository (or any parent up to mount point %s)\n"
			      "Stopping at filesystem boundary (GIT_DISCOVERY_ACROSS_FILESYSTEM not set)."),
			    dir.buf);
		*nongit_ok = 1;
		break;
	case GIT_DIR_INVALID_OWNERSHIP:
		if (!nongit_ok) {
			struct strbuf quoted = STRBUF_INIT;

			sq_quote_buf_pretty(&quoted, dir.buf);
			die(_("unsafe repository ('%s' is owned by someone else)\n"
			      "To add an exception for this directory, call:\n"
			      "\n"
			      "\tgit config --global --add safe.directory %s"),
			    dir.buf, quoted.buf);
		}
		*nongit_ok = 1;
		break;
	case GIT_DIR_NONE:
		/*
		 * As a safeguard against setup_git_directory_gently_1 returning
		 * this value, fallthrough to BUG. Otherwise it is possible to
		 * set startup_info->have_repository to 1 when we did nothing to
		 * find a repository.
		 */
	default:
		BUG("unhandled setup_git_directory_1() result");
	}

	/*
	 * At this point, nongit_ok is stable. If it is non-NULL and points
	 * to a non-zero value, then this means that we haven't found a
	 * repository and that the caller expects startup_info to reflect
	 * this.
	 *
	 * Regardless of the state of nongit_ok, startup_info->prefix and
	 * the GIT_PREFIX environment variable must always match. For details
	 * see Documentation/config/alias.txt.
	 */
	if (nongit_ok && *nongit_ok) {
		startup_info->have_repository = 0;
		startup_info->prefix = NULL;
		setenv(GIT_PREFIX_ENVIRONMENT, "", 1);
	} else {
		startup_info->have_repository = 1;
		startup_info->prefix = prefix;
		if (prefix)
			setenv(GIT_PREFIX_ENVIRONMENT, prefix, 1);
		else
			setenv(GIT_PREFIX_ENVIRONMENT, "", 1);
	}

	/*
	 * Not all paths through the setup code will call 'set_git_dir()' (which
	 * directly sets up the environment) so in order to guarantee that the
	 * environment is in a consistent state after setup, explicitly setup
	 * the environment if we have a repository.
	 *
	 * NEEDSWORK: currently we allow bogus GIT_DIR values to be set in some
	 * code paths so we also need to explicitly setup the environment if
	 * the user has set GIT_DIR.  It may be beneficial to disallow bogus
	 * GIT_DIR values at some point in the future.
	 */
	if (/* GIT_DIR_EXPLICIT, GIT_DIR_DISCOVERED, GIT_DIR_BARE */
	    startup_info->have_repository ||
	    /* GIT_DIR_EXPLICIT */
	    getenv(GIT_DIR_ENVIRONMENT)) {
		if (!the_repository->gitdir) {
			const char *gitdir = getenv(GIT_DIR_ENVIRONMENT);
			if (!gitdir)
				gitdir = DEFAULT_GIT_DIR_ENVIRONMENT;
			setup_git_env(gitdir);
		}
		if (startup_info->have_repository)
			repo_set_hash_algo(the_repository, repo_fmt.hash_algo);
	}

	strbuf_release(&dir);
	strbuf_release(&gitdir);
	clear_repository_format(&repo_fmt);

	return prefix;
}