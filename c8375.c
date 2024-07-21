static int checkout(int submodule_progress)
{
	struct object_id oid;
	char *head;
	struct lock_file *lock_file;
	struct unpack_trees_options opts;
	struct tree *tree;
	struct tree_desc t;
	int err = 0;

	if (option_no_checkout)
		return 0;

	head = resolve_refdup("HEAD", RESOLVE_REF_READING, oid.hash, NULL);
	if (!head) {
		warning(_("remote HEAD refers to nonexistent ref, "
			  "unable to checkout.\n"));
		return 0;
	}
	if (!strcmp(head, "HEAD")) {
		if (advice_detached_head)
			detach_advice(oid_to_hex(&oid));
	} else {
		if (!starts_with(head, "refs/heads/"))
			die(_("HEAD not found below refs/heads!"));
	}
	free(head);

	/* We need to be in the new work tree for the checkout */
	setup_work_tree();

	lock_file = xcalloc(1, sizeof(struct lock_file));
	hold_locked_index(lock_file, LOCK_DIE_ON_ERROR);

	memset(&opts, 0, sizeof opts);
	opts.update = 1;
	opts.merge = 1;
	opts.fn = oneway_merge;
	opts.verbose_update = (option_verbosity >= 0);
	opts.src_index = &the_index;
	opts.dst_index = &the_index;

	tree = parse_tree_indirect(&oid);
	parse_tree(tree);
	init_tree_desc(&t, tree->buffer, tree->size);
	if (unpack_trees(1, &t, &opts) < 0)
		die(_("unable to checkout working tree"));

	if (write_locked_index(&the_index, lock_file, COMMIT_LOCK))
		die(_("unable to write new index file"));

	err |= run_hook_le(NULL, "post-checkout", sha1_to_hex(null_sha1),
			   oid_to_hex(&oid), "1", NULL);

	if (!err && (option_recurse_submodules.nr > 0)) {
		struct argv_array args = ARGV_ARRAY_INIT;
		argv_array_pushl(&args, "submodule", "update", "--init", "--recursive", NULL);

		if (option_shallow_submodules == 1)
			argv_array_push(&args, "--depth=1");

		if (max_jobs != -1)
			argv_array_pushf(&args, "--jobs=%d", max_jobs);

		if (submodule_progress)
			argv_array_push(&args, "--progress");

		if (option_verbosity < 0)
			argv_array_push(&args, "--quiet");

		err = run_command_v_opt(args.argv, RUN_GIT_CMD);
		argv_array_clear(&args);
	}

	return err;
}