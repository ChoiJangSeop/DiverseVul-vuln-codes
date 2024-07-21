static int prepare_to_clone_next_submodule(const struct cache_entry *ce,
					   struct child_process *child,
					   struct submodule_update_clone *suc,
					   struct strbuf *out)
{
	const struct submodule *sub = NULL;
	struct strbuf displaypath_sb = STRBUF_INIT;
	struct strbuf sb = STRBUF_INIT;
	const char *displaypath = NULL;
	int needs_cloning = 0;

	if (ce_stage(ce)) {
		if (suc->recursive_prefix)
			strbuf_addf(&sb, "%s/%s", suc->recursive_prefix, ce->name);
		else
			strbuf_addstr(&sb, ce->name);
		strbuf_addf(out, _("Skipping unmerged submodule %s"), sb.buf);
		strbuf_addch(out, '\n');
		goto cleanup;
	}

	sub = submodule_from_path(null_sha1, ce->name);

	if (suc->recursive_prefix)
		displaypath = relative_path(suc->recursive_prefix,
					    ce->name, &displaypath_sb);
	else
		displaypath = ce->name;

	if (!sub) {
		next_submodule_warn_missing(suc, out, displaypath);
		goto cleanup;
	}

	if (suc->update.type == SM_UPDATE_NONE
	    || (suc->update.type == SM_UPDATE_UNSPECIFIED
		&& sub->update_strategy.type == SM_UPDATE_NONE)) {
		strbuf_addf(out, _("Skipping submodule '%s'"), displaypath);
		strbuf_addch(out, '\n');
		goto cleanup;
	}

	/* Check if the submodule has been initialized. */
	if (!is_submodule_active(the_repository, ce->name)) {
		next_submodule_warn_missing(suc, out, displaypath);
		goto cleanup;
	}

	strbuf_reset(&sb);
	strbuf_addf(&sb, "%s/.git", ce->name);
	needs_cloning = !file_exists(sb.buf);

	strbuf_reset(&sb);
	strbuf_addf(&sb, "%06o %s %d %d\t%s\n", ce->ce_mode,
			oid_to_hex(&ce->oid), ce_stage(ce),
			needs_cloning, ce->name);
	string_list_append(&suc->projectlines, sb.buf);

	if (!needs_cloning)
		goto cleanup;

	child->git_cmd = 1;
	child->no_stdin = 1;
	child->stdout_to_stderr = 1;
	child->err = -1;
	argv_array_push(&child->args, "submodule--helper");
	argv_array_push(&child->args, "clone");
	if (suc->progress)
		argv_array_push(&child->args, "--progress");
	if (suc->quiet)
		argv_array_push(&child->args, "--quiet");
	if (suc->prefix)
		argv_array_pushl(&child->args, "--prefix", suc->prefix, NULL);
	if (suc->recommend_shallow && sub->recommend_shallow == 1)
		argv_array_push(&child->args, "--depth=1");
	argv_array_pushl(&child->args, "--path", sub->path, NULL);
	argv_array_pushl(&child->args, "--name", sub->name, NULL);
	argv_array_pushl(&child->args, "--url", sub->url, NULL);
	if (suc->references.nr) {
		struct string_list_item *item;
		for_each_string_list_item(item, &suc->references)
			argv_array_pushl(&child->args, "--reference", item->string, NULL);
	}
	if (suc->depth)
		argv_array_push(&child->args, suc->depth);

cleanup:
	strbuf_reset(&displaypath_sb);
	strbuf_reset(&sb);

	return needs_cloning;
}