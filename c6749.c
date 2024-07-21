amstar_backup(
    application_argument_t *argument)
{
    int        dumpin;
    char      *cmd = NULL;
    char      *qdisk;
    char       line[32768];
    amregex_t *rp;
    off_t      dump_size = -1;
    char      *type;
    char       startchr;
    GPtrArray *argv_ptr;
    int        starpid;
    int        dataf = 1;
    int        mesgf = 3;
    int        indexf = 4;
    int        outf;
    FILE      *mesgstream;
    FILE      *indexstream = NULL;
    FILE      *outstream;
    int        level;
    regex_t    regex_root;
    regex_t    regex_dir;
    regex_t    regex_file;
    regex_t    regex_special;
    regex_t    regex_symbolic;
    regex_t    regex_hard;

    mesgstream = fdopen(mesgf, "w");
    if (!mesgstream) {
	error(_("error mesgstream(%d): %s\n"), mesgf, strerror(errno));
    }

    if (!argument->level) {
	fprintf(mesgstream, "? No level argument\n");
	error(_("No level argument"));
    }
    if (!argument->dle.disk) {
	fprintf(mesgstream, "? No disk argument\n");
	error(_("No disk argument"));
    }
    if (!argument->dle.device) {
	fprintf(mesgstream, "? No device argument\n");
	error(_("No device argument"));
    }

    if (!check_exec_for_suid(star_path, FALSE)) {
	fprintf(mesgstream, "? '%s' binary is not secure", star_path);
	error("'%s' binary is not secure", star_path);
    }

    if (argument->dle.include_list &&
	argument->dle.include_list->nb_element >= 0) {
	fprintf(mesgstream, "? include-list not supported for backup\n");
    }

    level = GPOINTER_TO_INT(argument->level->data);

    qdisk = quote_string(argument->dle.disk);

    argv_ptr = amstar_build_argv(argument, level, CMD_BACKUP, mesgstream);

    cmd = g_strdup(star_path);

    starpid = pipespawnv(cmd, STDIN_PIPE|STDERR_PIPE, 1,
			 &dumpin, &dataf, &outf, (char **)argv_ptr->pdata);

    g_ptr_array_free_full(argv_ptr);
    /* close the write ends of the pipes */
    aclose(dumpin);
    aclose(dataf);
    if (argument->dle.create_index) {
	indexstream = fdopen(indexf, "w");
	if (!indexstream) {
	    error(_("error indexstream(%d): %s\n"), indexf, strerror(errno));
	}
    }
    outstream = fdopen(outf, "r");
    if (!outstream) {
	error(_("error outstream(%d): %s\n"), outf, strerror(errno));
    }

    regcomp(&regex_root, "^a \\.\\/ directory$", REG_EXTENDED|REG_NEWLINE);
    regcomp(&regex_dir, "^a (.*) directory$", REG_EXTENDED|REG_NEWLINE);
    regcomp(&regex_file, "^a (.*) (.*) bytes", REG_EXTENDED|REG_NEWLINE);
    regcomp(&regex_special, "^a (.*) special", REG_EXTENDED|REG_NEWLINE);
    regcomp(&regex_symbolic, "^a (.*) symbolic", REG_EXTENDED|REG_NEWLINE);
    regcomp(&regex_hard, "^a (.*) link to", REG_EXTENDED|REG_NEWLINE);

    while ((fgets(line, sizeof(line), outstream)) != NULL) {
	regmatch_t regmatch[3];

	if (strlen(line) > 0 && line[strlen(line)-1] == '\n') {
	    /* remove trailling \n */
	    line[strlen(line)-1] = '\0';
	}

	if (regexec(&regex_root, line, 1, regmatch, 0) == 0) {
	    if (argument->dle.create_index)
		fprintf(indexstream, "%s\n", "/");
	    continue;
	}

	if (regexec(&regex_dir, line, 3, regmatch, 0) == 0) {
	    if (argument->dle.create_index && regmatch[1].rm_so == 2) {
		line[regmatch[1].rm_eo]='\0';
		fprintf(indexstream, "/%s\n", &line[regmatch[1].rm_so]);
	    }
	    continue;
	}

	if (regexec(&regex_file, line, 3, regmatch, 0) == 0 ||
	    regexec(&regex_special, line, 3, regmatch, 0) == 0 ||
	    regexec(&regex_symbolic, line, 3, regmatch, 0) == 0 ||
	    regexec(&regex_hard, line, 3, regmatch, 0) == 0) {
	    if (argument->dle.create_index && regmatch[1].rm_so == 2) {
		line[regmatch[1].rm_eo]='\0';
		fprintf(indexstream, "/%s\n", &line[regmatch[1].rm_so]);
	    }
	    continue;
	}

	for (rp = re_table; rp->regex != NULL; rp++) {
	    if (match(rp->regex, line)) {
		break;
	    }
	}
	if (rp->typ == DMP_SIZE) {
	    dump_size = (off_t)((the_num(line, rp->field)* rp->scale+1023.0)/1024.0);
	}
	switch (rp->typ) {
	    case DMP_IGNORE:
		continue;
	    case DMP_NORMAL:
		type = "normal";
		startchr = '|';
		break;
	    case DMP_STRANGE:
		type = "strange";
		startchr = '?';
		break;
	    case DMP_SIZE:
		type = "size";
		startchr = '|';
		break;
	    case DMP_ERROR:
		type = "error";
		startchr = '?';
		break;
	    default:
		type = "unknown";
		startchr = '!';
		break;
	}
	dbprintf("%3d: %7s(%c): %s\n", rp->srcline, type, startchr, line);
	fprintf(mesgstream,"%c %s\n", startchr, line);
    }
    fclose(outstream);

    regfree(&regex_root);
    regfree(&regex_dir);
    regfree(&regex_file);
    regfree(&regex_special);
    regfree(&regex_symbolic);
    regfree(&regex_hard);

    dbprintf(_("gnutar: %s: pid %ld\n"), cmd, (long)starpid);

    dbprintf("sendbackup: size %lld\n", (long long)dump_size);
    fprintf(mesgstream, "sendbackup: size %lld\n", (long long)dump_size);

    fclose(mesgstream);
    if (argument->dle.create_index)
	fclose(indexstream);

    amfree(qdisk);
    amfree(cmd);
}