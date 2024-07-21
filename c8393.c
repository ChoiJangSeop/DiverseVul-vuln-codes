int cmd_main(int argc, const char **argv)
{
	unsigned int i;

	if (argc == 2 && !strcmp(argv[1], "-h"))
		usage(fast_import_usage);

	setup_git_directory();
	reset_pack_idx_option(&pack_idx_opts);
	git_pack_config();

	alloc_objects(object_entry_alloc);
	strbuf_init(&command_buf, 0);
	atom_table = xcalloc(atom_table_sz, sizeof(struct atom_str*));
	branch_table = xcalloc(branch_table_sz, sizeof(struct branch*));
	avail_tree_table = xcalloc(avail_tree_table_sz, sizeof(struct avail_tree_content*));
	marks = pool_calloc(1, sizeof(struct mark_set));

	global_argc = argc;
	global_argv = argv;

	rc_free = pool_alloc(cmd_save * sizeof(*rc_free));
	for (i = 0; i < (cmd_save - 1); i++)
		rc_free[i].next = &rc_free[i + 1];
	rc_free[cmd_save - 1].next = NULL;

	prepare_packed_git();
	start_packfile();
	set_die_routine(die_nicely);
	set_checkpoint_signal();
	while (read_next_command() != EOF) {
		const char *v;
		if (!strcmp("blob", command_buf.buf))
			parse_new_blob();
		else if (skip_prefix(command_buf.buf, "ls ", &v))
			parse_ls(v, NULL);
		else if (skip_prefix(command_buf.buf, "commit ", &v))
			parse_new_commit(v);
		else if (skip_prefix(command_buf.buf, "tag ", &v))
			parse_new_tag(v);
		else if (skip_prefix(command_buf.buf, "reset ", &v))
			parse_reset_branch(v);
		else if (!strcmp("checkpoint", command_buf.buf))
			parse_checkpoint();
		else if (!strcmp("done", command_buf.buf))
			break;
		else if (starts_with(command_buf.buf, "progress "))
			parse_progress();
		else if (skip_prefix(command_buf.buf, "feature ", &v))
			parse_feature(v);
		else if (skip_prefix(command_buf.buf, "option git ", &v))
			parse_option(v);
		else if (starts_with(command_buf.buf, "option "))
			/* ignore non-git options*/;
		else
			die("Unsupported command: %s", command_buf.buf);

		if (checkpoint_requested)
			checkpoint();
	}

	/* argv hasn't been parsed yet, do so */
	if (!seen_data_command)
		parse_argv();

	if (require_explicit_termination && feof(stdin))
		die("stream ends early");

	end_packfile();

	dump_branches();
	dump_tags();
	unkeep_all_packs();
	dump_marks();

	if (pack_edges)
		fclose(pack_edges);

	if (show_stats) {
		uintmax_t total_count = 0, duplicate_count = 0;
		for (i = 0; i < ARRAY_SIZE(object_count_by_type); i++)
			total_count += object_count_by_type[i];
		for (i = 0; i < ARRAY_SIZE(duplicate_count_by_type); i++)
			duplicate_count += duplicate_count_by_type[i];

		fprintf(stderr, "%s statistics:\n", argv[0]);
		fprintf(stderr, "---------------------------------------------------------------------\n");
		fprintf(stderr, "Alloc'd objects: %10" PRIuMAX "\n", alloc_count);
		fprintf(stderr, "Total objects:   %10" PRIuMAX " (%10" PRIuMAX " duplicates                  )\n", total_count, duplicate_count);
		fprintf(stderr, "      blobs  :   %10" PRIuMAX " (%10" PRIuMAX " duplicates %10" PRIuMAX " deltas of %10" PRIuMAX" attempts)\n", object_count_by_type[OBJ_BLOB], duplicate_count_by_type[OBJ_BLOB], delta_count_by_type[OBJ_BLOB], delta_count_attempts_by_type[OBJ_BLOB]);
		fprintf(stderr, "      trees  :   %10" PRIuMAX " (%10" PRIuMAX " duplicates %10" PRIuMAX " deltas of %10" PRIuMAX" attempts)\n", object_count_by_type[OBJ_TREE], duplicate_count_by_type[OBJ_TREE], delta_count_by_type[OBJ_TREE], delta_count_attempts_by_type[OBJ_TREE]);
		fprintf(stderr, "      commits:   %10" PRIuMAX " (%10" PRIuMAX " duplicates %10" PRIuMAX " deltas of %10" PRIuMAX" attempts)\n", object_count_by_type[OBJ_COMMIT], duplicate_count_by_type[OBJ_COMMIT], delta_count_by_type[OBJ_COMMIT], delta_count_attempts_by_type[OBJ_COMMIT]);
		fprintf(stderr, "      tags   :   %10" PRIuMAX " (%10" PRIuMAX " duplicates %10" PRIuMAX " deltas of %10" PRIuMAX" attempts)\n", object_count_by_type[OBJ_TAG], duplicate_count_by_type[OBJ_TAG], delta_count_by_type[OBJ_TAG], delta_count_attempts_by_type[OBJ_TAG]);
		fprintf(stderr, "Total branches:  %10lu (%10lu loads     )\n", branch_count, branch_load_count);
		fprintf(stderr, "      marks:     %10" PRIuMAX " (%10" PRIuMAX " unique    )\n", (((uintmax_t)1) << marks->shift) * 1024, marks_set_count);
		fprintf(stderr, "      atoms:     %10u\n", atom_cnt);
		fprintf(stderr, "Memory total:    %10" PRIuMAX " KiB\n", (total_allocd + alloc_count*sizeof(struct object_entry))/1024);
		fprintf(stderr, "       pools:    %10lu KiB\n", (unsigned long)(total_allocd/1024));
		fprintf(stderr, "     objects:    %10" PRIuMAX " KiB\n", (alloc_count*sizeof(struct object_entry))/1024);
		fprintf(stderr, "---------------------------------------------------------------------\n");
		pack_report();
		fprintf(stderr, "---------------------------------------------------------------------\n");
		fprintf(stderr, "\n");
	}

	return failure ? 1 : 0;
}