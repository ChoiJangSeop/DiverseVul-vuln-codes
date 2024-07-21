static int module_clone(int argc, const char **argv, const char *prefix)
{
	const char *name = NULL, *url = NULL, *depth = NULL;
	int quiet = 0;
	int progress = 0;
	char *p, *path = NULL, *sm_gitdir;
	struct strbuf sb = STRBUF_INIT;
	struct string_list reference = STRING_LIST_INIT_NODUP;
	int require_init = 0;
	char *sm_alternate = NULL, *error_strategy = NULL;

	struct option module_clone_options[] = {
		OPT_STRING(0, "prefix", &prefix,
			   N_("path"),
			   N_("alternative anchor for relative paths")),
		OPT_STRING(0, "path", &path,
			   N_("path"),
			   N_("where the new submodule will be cloned to")),
		OPT_STRING(0, "name", &name,
			   N_("string"),
			   N_("name of the new submodule")),
		OPT_STRING(0, "url", &url,
			   N_("string"),
			   N_("url where to clone the submodule from")),
		OPT_STRING_LIST(0, "reference", &reference,
			   N_("repo"),
			   N_("reference repository")),
		OPT_STRING(0, "depth", &depth,
			   N_("string"),
			   N_("depth for shallow clones")),
		OPT__QUIET(&quiet, "Suppress output for cloning a submodule"),
		OPT_BOOL(0, "progress", &progress,
			   N_("force cloning progress")),
		OPT_BOOL(0, "require-init", &require_init,
			   N_("disallow cloning into non-empty directory")),
		OPT_END()
	};

	const char *const git_submodule_helper_usage[] = {
		N_("git submodule--helper clone [--prefix=<path>] [--quiet] "
		   "[--reference <repository>] [--name <name>] [--depth <depth>] "
		   "--url <url> --path <path>"),
		NULL
	};

	argc = parse_options(argc, argv, prefix, module_clone_options,
			     git_submodule_helper_usage, 0);

	if (argc || !url || !path || !*path)
		usage_with_options(git_submodule_helper_usage,
				   module_clone_options);

	strbuf_addf(&sb, "%s/modules/%s", get_git_dir(), name);
	sm_gitdir = absolute_pathdup(sb.buf);
	strbuf_reset(&sb);

	if (!is_absolute_path(path)) {
		strbuf_addf(&sb, "%s/%s", get_git_work_tree(), path);
		path = strbuf_detach(&sb, NULL);
	} else
		path = xstrdup(path);

	if (!file_exists(sm_gitdir)) {
		if (safe_create_leading_directories_const(sm_gitdir) < 0)
			die(_("could not create directory '%s'"), sm_gitdir);

		prepare_possible_alternates(name, &reference);

		if (clone_submodule(path, sm_gitdir, url, depth, &reference,
				    quiet, progress))
			die(_("clone of '%s' into submodule path '%s' failed"),
			    url, path);
	} else {
		if (require_init && !access(path, X_OK) && !is_empty_dir(path))
			die(_("directory not empty: '%s'"), path);
		if (safe_create_leading_directories_const(path) < 0)
			die(_("could not create directory '%s'"), path);
		strbuf_addf(&sb, "%s/index", sm_gitdir);
		unlink_or_warn(sb.buf);
		strbuf_reset(&sb);
	}

	/* Connect module worktree and git dir */
	connect_work_tree_and_git_dir(path, sm_gitdir);

	p = git_pathdup_submodule(path, "config");
	if (!p)
		die(_("could not get submodule directory for '%s'"), path);

	/* setup alternateLocation and alternateErrorStrategy in the cloned submodule if needed */
	git_config_get_string("submodule.alternateLocation", &sm_alternate);
	if (sm_alternate)
		git_config_set_in_file(p, "submodule.alternateLocation",
					   sm_alternate);
	git_config_get_string("submodule.alternateErrorStrategy", &error_strategy);
	if (error_strategy)
		git_config_set_in_file(p, "submodule.alternateErrorStrategy",
					   error_strategy);

	free(sm_alternate);
	free(error_strategy);

	strbuf_release(&sb);
	free(sm_gitdir);
	free(path);
	free(p);
	return 0;
}