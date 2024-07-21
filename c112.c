int run_diff_files(struct rev_info *revs, unsigned int option)
{
	int entries, i;
	int diff_unmerged_stage = revs->max_count;
	int silent_on_removed = option & DIFF_SILENT_ON_REMOVED;
	unsigned ce_option = ((option & DIFF_RACY_IS_MODIFIED)
			      ? CE_MATCH_RACY_IS_DIRTY : 0);
	char symcache[PATH_MAX];

	if (diff_unmerged_stage < 0)
		diff_unmerged_stage = 2;
	entries = active_nr;
	symcache[0] = '\0';
	for (i = 0; i < entries; i++) {
		struct stat st;
		unsigned int oldmode, newmode;
		struct cache_entry *ce = active_cache[i];
		int changed;

		if (DIFF_OPT_TST(&revs->diffopt, QUIET) &&
			DIFF_OPT_TST(&revs->diffopt, HAS_CHANGES))
			break;

		if (!ce_path_match(ce, revs->prune_data))
			continue;

		if (ce_stage(ce)) {
			struct combine_diff_path *dpath;
			int num_compare_stages = 0;
			size_t path_len;

			path_len = ce_namelen(ce);

			dpath = xmalloc(combine_diff_path_size(5, path_len));
			dpath->path = (char *) &(dpath->parent[5]);

			dpath->next = NULL;
			dpath->len = path_len;
			memcpy(dpath->path, ce->name, path_len);
			dpath->path[path_len] = '\0';
			hashclr(dpath->sha1);
			memset(&(dpath->parent[0]), 0,
			       sizeof(struct combine_diff_parent)*5);

			changed = check_removed(ce, &st);
			if (!changed)
				dpath->mode = ce_mode_from_stat(ce, st.st_mode);
			else {
				if (changed < 0) {
					perror(ce->name);
					continue;
				}
				if (silent_on_removed)
					continue;
			}

			while (i < entries) {
				struct cache_entry *nce = active_cache[i];
				int stage;

				if (strcmp(ce->name, nce->name))
					break;

				/* Stage #2 (ours) is the first parent,
				 * stage #3 (theirs) is the second.
				 */
				stage = ce_stage(nce);
				if (2 <= stage) {
					int mode = nce->ce_mode;
					num_compare_stages++;
					hashcpy(dpath->parent[stage-2].sha1, nce->sha1);
					dpath->parent[stage-2].mode = ce_mode_from_stat(nce, mode);
					dpath->parent[stage-2].status =
						DIFF_STATUS_MODIFIED;
				}

				/* diff against the proper unmerged stage */
				if (stage == diff_unmerged_stage)
					ce = nce;
				i++;
			}
			/*
			 * Compensate for loop update
			 */
			i--;

			if (revs->combine_merges && num_compare_stages == 2) {
				show_combined_diff(dpath, 2,
						   revs->dense_combined_merges,
						   revs);
				free(dpath);
				continue;
			}
			free(dpath);
			dpath = NULL;

			/*
			 * Show the diff for the 'ce' if we found the one
			 * from the desired stage.
			 */
			diff_unmerge(&revs->diffopt, ce->name, 0, null_sha1);
			if (ce_stage(ce) != diff_unmerged_stage)
				continue;
		}

		if (ce_uptodate(ce))
			continue;

		changed = check_removed(ce, &st);
		if (changed) {
			if (changed < 0) {
				perror(ce->name);
				continue;
			}
			if (silent_on_removed)
				continue;
			diff_addremove(&revs->diffopt, '-', ce->ce_mode,
				       ce->sha1, ce->name, NULL);
			continue;
		}
		changed = ce_match_stat(ce, &st, ce_option);
		if (!changed) {
			ce_mark_uptodate(ce);
			if (!DIFF_OPT_TST(&revs->diffopt, FIND_COPIES_HARDER))
				continue;
		}
		oldmode = ce->ce_mode;
		newmode = ce_mode_from_stat(ce, st.st_mode);
		diff_change(&revs->diffopt, oldmode, newmode,
			    ce->sha1, (changed ? null_sha1 : ce->sha1),
			    ce->name, NULL);

	}
	diffcore_std(&revs->diffopt);
	diff_flush(&revs->diffopt);
	return 0;
}