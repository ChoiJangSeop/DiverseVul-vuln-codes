void init_revisions(struct rev_info *revs, const char *prefix)
{
	memset(revs, 0, sizeof(*revs));

	revs->abbrev = DEFAULT_ABBREV;
	revs->ignore_merges = 1;
	revs->simplify_history = 1;
	DIFF_OPT_SET(&revs->pruning, RECURSIVE);
	DIFF_OPT_SET(&revs->pruning, QUICK);
	revs->pruning.add_remove = file_add_remove;
	revs->pruning.change = file_change;
	revs->sort_order = REV_SORT_IN_GRAPH_ORDER;
	revs->dense = 1;
	revs->prefix = prefix;
	revs->max_age = -1;
	revs->min_age = -1;
	revs->skip_count = -1;
	revs->max_count = -1;
	revs->max_parents = -1;
	revs->expand_tabs_in_log = -1;

	revs->commit_format = CMIT_FMT_DEFAULT;
	revs->expand_tabs_in_log_default = 8;

	init_grep_defaults();
	grep_init(&revs->grep_filter, prefix);
	revs->grep_filter.status_only = 1;
	revs->grep_filter.regflags = REG_NEWLINE;

	diff_setup(&revs->diffopt);
	if (prefix && !revs->diffopt.prefix) {
		revs->diffopt.prefix = prefix;
		revs->diffopt.prefix_length = strlen(prefix);
	}

	revs->notes_opt.use_default_notes = -1;
}