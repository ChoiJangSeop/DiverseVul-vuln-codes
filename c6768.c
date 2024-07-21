amstar_restore(
    application_argument_t *argument)
{
    char       *cmd;
    GPtrArray  *argv_ptr = g_ptr_array_new();
    char      **env;
    int         j;
    char       *e;

    if (!star_path) {
	error(_("STAR-PATH not defined"));
    }
    if (!check_exec_for_suid(star_path, FALSE)) {
	error("'%s' binary is not secure", star_path);
    }

    cmd = stralloc(star_path);

    g_ptr_array_add(argv_ptr, stralloc(star_path));
    if (star_directory) {
	struct stat stat_buf;
	if(stat(star_directory, &stat_buf) != 0) {
	    fprintf(stderr,"can not stat directory %s: %s\n", star_directory, strerror(errno));
	    exit(1);
	}
	if (!S_ISDIR(stat_buf.st_mode)) {
	    fprintf(stderr,"%s is not a directory\n", star_directory);
	    exit(1);
	}
	if (access(star_directory, W_OK) != 0 ) {
	    fprintf(stderr, "Can't write to %s: %s\n", star_directory, strerror(errno));
	    exit(1);
	}

	g_ptr_array_add(argv_ptr, stralloc("-C"));
	g_ptr_array_add(argv_ptr, stralloc(star_directory));
    }
    g_ptr_array_add(argv_ptr, stralloc("-x"));
    g_ptr_array_add(argv_ptr, stralloc("-v"));
    g_ptr_array_add(argv_ptr, stralloc("-xattr"));
    g_ptr_array_add(argv_ptr, stralloc("-acl"));
    g_ptr_array_add(argv_ptr, stralloc("errctl=WARN|SAMEFILE|SETTIME|DIFF|SETACL|SETXATTR|SETMODE|BADACL *"));
    g_ptr_array_add(argv_ptr, stralloc("-no-fifo"));
    g_ptr_array_add(argv_ptr, stralloc("-f"));
    g_ptr_array_add(argv_ptr, stralloc("-"));

    if (argument->dle.exclude_list &&
	argument->dle.exclude_list->nb_element == 1) {
	g_ptr_array_add(argv_ptr, stralloc("-exclude-from"));
	g_ptr_array_add(argv_ptr,
			stralloc(argument->dle.exclude_list->first->name));
    }

    if (argument->dle.include_list &&
	argument->dle.include_list->nb_element == 1) {
	FILE *include_list = fopen(argument->dle.include_list->first->name, "r");
	char  line[2*PATH_MAX+2];
	while (fgets(line, 2*PATH_MAX, include_list)) {
	    line[strlen(line)-1] = '\0'; /* remove '\n' */
	    if (strncmp(line, "./", 2) == 0)
		g_ptr_array_add(argv_ptr, stralloc(line+2)); /* remove ./ */
	    else if (strcmp(line, ".") != 0)
		g_ptr_array_add(argv_ptr, stralloc(line));
	}
	fclose(include_list);
    }
    for (j=1; j< argument->argc; j++) {
	if (strncmp(argument->argv[j], "./", 2) == 0)
	    g_ptr_array_add(argv_ptr, stralloc(argument->argv[j]+2));/*remove ./ */
	else if (strcmp(argument->argv[j], ".") != 0)
	    g_ptr_array_add(argv_ptr, stralloc(argument->argv[j]));
    }
    g_ptr_array_add(argv_ptr, NULL);

    debug_executing(argv_ptr);
    env = safe_env();
    become_root();
    execve(cmd, (char **)argv_ptr->pdata, env);
    e = strerror(errno);
    error(_("error [exec %s: %s]"), cmd, e);

}