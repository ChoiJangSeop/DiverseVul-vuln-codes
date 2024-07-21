amgtar_backup(
    application_argument_t *argument)
{
    int         dumpin;
    char      *cmd = NULL;
    char      *qdisk;
    char      *incrname;
    char       line[32768];
    amregex_t *rp;
    off_t      dump_size = -1;
    char      *type;
    char       startchr;
    int        dataf = 1;
    int        mesgf = 3;
    int        indexf = 4;
    int        outf;
    FILE      *mesgstream;
    FILE      *indexstream = NULL;
    FILE      *outstream;
    char      *errmsg = NULL;
    amwait_t   wait_status;
    GPtrArray *argv_ptr;
    int        tarpid;
    char      *file_exclude;
    char      *file_include;
    char      *option;

    mesgstream = fdopen(mesgf, "w");
    if (!mesgstream) {
	error(_("error mesgstream(%d): %s\n"), mesgf, strerror(errno));
    }

    if (!gnutar_path) {
	error(_("GNUTAR-PATH not defined"));
    }
    if (!gnutar_listdir) {
	error(_("GNUTAR-LISTDIR not defined"));
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

    if (!check_exec_for_suid(gnutar_path, FALSE)) {
	fprintf(mesgstream, "? '%s' binary is not secure\n", gnutar_path);
	error("'%s' binary is not secure", gnutar_path);
    }

    if ((option = validate_command_options(argument))) {
	fprintf(stderr, "? Invalid '%s' COMMAND-OPTIONS\n", option);
	error("Invalid '%s' COMMAND-OPTIONS", option);
    }

    qdisk = quote_string(argument->dle.disk);

    incrname = amgtar_get_incrname(argument,
				   GPOINTER_TO_INT(argument->level->data),
				   mesgstream, CMD_BACKUP);
    cmd = stralloc(gnutar_path);
    argv_ptr = amgtar_build_argv(argument, incrname, &file_exclude,
				 &file_include, CMD_BACKUP);

    tarpid = pipespawnv(cmd, STDIN_PIPE|STDERR_PIPE, 1,
			&dumpin, &dataf, &outf, (char **)argv_ptr->pdata);
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

    while (fgets(line, sizeof(line), outstream) != NULL) {
	if (line[strlen(line)-1] == '\n') /* remove trailling \n */
	    line[strlen(line)-1] = '\0';
	if (*line == '.' && *(line+1) == '/') { /* filename */
	    if (argument->dle.create_index) {
		fprintf(indexstream, "%s\n", &line[1]); /* remove . */
	    }
	} else { /* message */
	    for(rp = re_table; rp->regex != NULL; rp++) {
		if(match(rp->regex, line)) {
		    break;
		}
	    }
	    if(rp->typ == DMP_SIZE) {
		dump_size = (off_t)((the_num(line, rp->field)* rp->scale+1023.0)/1024.0);
	    }
	    switch(rp->typ) {
	    case DMP_NORMAL:
		type = "normal";
		startchr = '|';
		break;
	    case DMP_IGNORE:
		continue;
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
    }

    waitpid(tarpid, &wait_status, 0);
    if (WIFSIGNALED(wait_status)) {
	errmsg = vstrallocf(_("%s terminated with signal %d: see %s"),
			    cmd, WTERMSIG(wait_status), dbfn());
    } else if (WIFEXITED(wait_status)) {
	if (exit_value[WEXITSTATUS(wait_status)] == 1) {
	    errmsg = vstrallocf(_("%s exited with status %d: see %s"),
				cmd, WEXITSTATUS(wait_status), dbfn());
	} else {
	    /* Normal exit */
	}
    } else {
	errmsg = vstrallocf(_("%s got bad exit: see %s"),
			    cmd, dbfn());
    }
    dbprintf(_("after %s %s wait\n"), cmd, qdisk);
    dbprintf(_("amgtar: %s: pid %ld\n"), cmd, (long)tarpid);
    if (errmsg) {
	dbprintf("%s", errmsg);
	g_fprintf(mesgstream, "sendbackup: error [%s]\n", errmsg);
    }

    if (!errmsg && incrname && strlen(incrname) > 4) {
	if (argument->dle.record) {
	    char *nodotnew;
	    nodotnew = stralloc(incrname);
	    nodotnew[strlen(nodotnew)-4] = '\0';
	    if (rename(incrname, nodotnew)) {
		dbprintf(_("%s: warning [renaming %s to %s: %s]\n"),
			 get_pname(), incrname, nodotnew, strerror(errno));
		g_fprintf(mesgstream, _("? warning [renaming %s to %s: %s]\n"),
			  incrname, nodotnew, strerror(errno));
	    }
	    amfree(nodotnew);
	} else {
	    if (unlink(incrname) == -1) {
		dbprintf(_("%s: warning [unlink %s: %s]\n"),
			 get_pname(), incrname, strerror(errno));
		g_fprintf(mesgstream, _("? warning [unlink %s: %s]\n"),
			  incrname, strerror(errno));
	    }
	}
    }

    dbprintf("sendbackup: size %lld\n", (long long)dump_size);
    fprintf(mesgstream, "sendbackup: size %lld\n", (long long)dump_size);
    dbprintf("sendbackup: end\n");
    fprintf(mesgstream, "sendbackup: end\n");

    if (argument->dle.create_index)
	fclose(indexstream);

    fclose(mesgstream);

    if (argument->verbose == 0) {
	if (file_exclude)
	    unlink(file_exclude);
	if (file_include)
	    unlink(file_include);
    }

    amfree(incrname);
    amfree(qdisk);
    amfree(cmd);
    g_ptr_array_free_full(argv_ptr);
}