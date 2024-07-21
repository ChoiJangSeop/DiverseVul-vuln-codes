static int show_modified(struct oneway_unpack_data *cbdata,
			 struct cache_entry *old,
			 struct cache_entry *new,
			 int report_missing,
			 int cached, int match_missing)
{
	unsigned int mode, oldmode;
	const unsigned char *sha1;
	struct rev_info *revs = cbdata->revs;

	if (get_stat_data(new, &sha1, &mode, cached, match_missing, cbdata) < 0) {
		if (report_missing)
			diff_index_show_file(revs, "-", old,
					     old->sha1, old->ce_mode);
		return -1;
	}

	if (revs->combine_merges && !cached &&
	    (hashcmp(sha1, old->sha1) || hashcmp(old->sha1, new->sha1))) {
		struct combine_diff_path *p;
		int pathlen = ce_namelen(new);

		p = xmalloc(combine_diff_path_size(2, pathlen));
		p->path = (char *) &p->parent[2];
		p->next = NULL;
		p->len = pathlen;
		memcpy(p->path, new->name, pathlen);
		p->path[pathlen] = 0;
		p->mode = mode;
		hashclr(p->sha1);
		memset(p->parent, 0, 2 * sizeof(struct combine_diff_parent));
		p->parent[0].status = DIFF_STATUS_MODIFIED;
		p->parent[0].mode = new->ce_mode;
		hashcpy(p->parent[0].sha1, new->sha1);
		p->parent[1].status = DIFF_STATUS_MODIFIED;
		p->parent[1].mode = old->ce_mode;
		hashcpy(p->parent[1].sha1, old->sha1);
		show_combined_diff(p, 2, revs->dense_combined_merges, revs);
		free(p);
		return 0;
	}

	oldmode = old->ce_mode;
	if (mode == oldmode && !hashcmp(sha1, old->sha1) &&
	    !DIFF_OPT_TST(&revs->diffopt, FIND_COPIES_HARDER))
		return 0;

	diff_change(&revs->diffopt, oldmode, mode,
		    old->sha1, sha1, old->name, NULL);
	return 0;
}