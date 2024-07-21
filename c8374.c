static int update_clone(int argc, const char **argv, const char *prefix)
{
	const char *update = NULL;
	int max_jobs = -1;
	struct string_list_item *item;
	struct pathspec pathspec;
	struct submodule_update_clone suc = SUBMODULE_UPDATE_CLONE_INIT;

	struct option module_update_clone_options[] = {
		OPT_STRING(0, "prefix", &prefix,
			   N_("path"),
			   N_("path into the working tree")),
		OPT_STRING(0, "recursive-prefix", &suc.recursive_prefix,
			   N_("path"),
			   N_("path into the working tree, across nested "
			      "submodule boundaries")),
		OPT_STRING(0, "update", &update,
			   N_("string"),
			   N_("rebase, merge, checkout or none")),
		OPT_STRING_LIST(0, "reference", &suc.references, N_("repo"),
			   N_("reference repository")),
		OPT_STRING(0, "depth", &suc.depth, "<depth>",
			   N_("Create a shallow clone truncated to the "
			      "specified number of revisions")),
		OPT_INTEGER('j', "jobs", &max_jobs,
			    N_("parallel jobs")),
		OPT_BOOL(0, "recommend-shallow", &suc.recommend_shallow,
			    N_("whether the initial clone should follow the shallow recommendation")),
		OPT__QUIET(&suc.quiet, N_("don't print cloning progress")),
		OPT_BOOL(0, "progress", &suc.progress,
			    N_("force cloning progress")),
		OPT_END()
	};

	const char *const git_submodule_helper_usage[] = {
		N_("git submodule--helper update_clone [--prefix=<path>] [<path>...]"),
		NULL
	};
	suc.prefix = prefix;

	argc = parse_options(argc, argv, prefix, module_update_clone_options,
			     git_submodule_helper_usage, 0);

	if (update)
		if (parse_submodule_update_strategy(update, &suc.update) < 0)
			die(_("bad value for update parameter"));

	if (module_list_compute(argc, argv, prefix, &pathspec, &suc.list) < 0)
		return 1;

	if (pathspec.nr)
		suc.warn_if_uninitialized = 1;

	/* Overlay the parsed .gitmodules file with .git/config */
	gitmodules_config();
	git_config(submodule_config, NULL);

	if (max_jobs < 0)
		max_jobs = parallel_submodules();

	run_processes_parallel(max_jobs,
			       update_clone_get_next_task,
			       update_clone_start_failure,
			       update_clone_task_finished,
			       &suc);

	/*
	 * We saved the output and put it out all at once now.
	 * That means:
	 * - the listener does not have to interleave their (checkout)
	 *   work with our fetching.  The writes involved in a
	 *   checkout involve more straightforward sequential I/O.
	 * - the listener can avoid doing any work if fetching failed.
	 */
	if (suc.quickstop)
		return 1;

	for_each_string_list_item(item, &suc.projectlines)
		fprintf(stdout, "%s", item->string);

	return 0;
}