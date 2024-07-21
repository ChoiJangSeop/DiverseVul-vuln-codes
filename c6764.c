static GPtrArray *amstar_build_argv(
    application_argument_t *argument,
    int   level,
    int   command,
    FILE *mesgstream)
{
    char      *dirname;
    char      *fsname;
    char       levelstr[NUM_STR_SIZE+7];
    GPtrArray *argv_ptr = g_ptr_array_new();
    char      *s;
    char      *tardumpfile;
    GSList    *copt;

    if (star_directory) {
	dirname = star_directory;
    } else {
	dirname = argument->dle.device;
    }
    fsname = vstralloc("fs-name=", dirname, NULL);
    for (s = fsname; *s != '\0'; s++) {
	if (iscntrl((int)*s))
	    *s = '-';
    }
    snprintf(levelstr, SIZEOF(levelstr), "-level=%d", level);

    if (star_dle_tardumps) {
	char *sdisk = sanitise_filename(argument->dle.disk);
	tardumpfile = vstralloc(star_tardumps, sdisk, NULL);
	amfree(sdisk);
    } else {
	tardumpfile = stralloc(star_tardumps);
    }

    g_ptr_array_add(argv_ptr, stralloc(star_path));

    g_ptr_array_add(argv_ptr, stralloc("-c"));
    g_ptr_array_add(argv_ptr, stralloc("-f"));
    if (command == CMD_ESTIMATE) {
	g_ptr_array_add(argv_ptr, stralloc("/dev/null"));
    } else {
	g_ptr_array_add(argv_ptr, stralloc("-"));
    }
    g_ptr_array_add(argv_ptr, stralloc("-C"));

#if defined(__CYGWIN__)
    {
	char tmppath[PATH_MAX];

	cygwin_conv_to_full_posix_path(dirname, tmppath);
	g_ptr_array_add(argv_ptr, stralloc(tmppath));
    }
#else
    g_ptr_array_add(argv_ptr, stralloc(dirname));
#endif
    g_ptr_array_add(argv_ptr, stralloc(fsname));
    if (star_onefilesystem)
	g_ptr_array_add(argv_ptr, stralloc("-xdev"));
    g_ptr_array_add(argv_ptr, stralloc("-link-dirs"));
    g_ptr_array_add(argv_ptr, stralloc(levelstr));
    g_ptr_array_add(argv_ptr, stralloc2("tardumps=", tardumpfile));
    if (command == CMD_BACKUP)
	g_ptr_array_add(argv_ptr, stralloc("-wtardumps"));

    g_ptr_array_add(argv_ptr, stralloc("-xattr"));
    if (star_acl)
	g_ptr_array_add(argv_ptr, stralloc("-acl"));
    g_ptr_array_add(argv_ptr, stralloc("H=exustar"));
    g_ptr_array_add(argv_ptr, stralloc("errctl=WARN|SAMEFILE|DIFF|GROW|SHRINK|SPECIALFILE|GETXATTR|BADACL *"));
    if (star_sparse)
	g_ptr_array_add(argv_ptr, stralloc("-sparse"));
    g_ptr_array_add(argv_ptr, stralloc("-dodesc"));

    if (command == CMD_BACKUP && argument->dle.create_index)
	g_ptr_array_add(argv_ptr, stralloc("-v"));

    if (((argument->dle.include_file &&
	  argument->dle.include_file->nb_element >= 1) ||
	 (argument->dle.include_list &&
	  argument->dle.include_list->nb_element >= 1)) &&
	((argument->dle.exclude_file &&
	  argument->dle.exclude_file->nb_element >= 1) ||
	 (argument->dle.exclude_list &&
	  argument->dle.exclude_list->nb_element >= 1))) {

	if (mesgstream && command == CMD_BACKUP) {
	    fprintf(mesgstream, "? include and exclude specified, disabling exclude\n");
	}
	free_sl(argument->dle.exclude_file);
	argument->dle.exclude_file = NULL;
	free_sl(argument->dle.exclude_list);
	argument->dle.exclude_list = NULL;
    }

    if ((argument->dle.exclude_file &&
	 argument->dle.exclude_file->nb_element >= 1) ||
	(argument->dle.exclude_list &&
	 argument->dle.exclude_list->nb_element >= 1)) {
	g_ptr_array_add(argv_ptr, stralloc("-match-tree"));
	g_ptr_array_add(argv_ptr, stralloc("-not"));
    }
    if (argument->dle.exclude_file &&
	argument->dle.exclude_file->nb_element >= 1) {
	sle_t *excl;
	for (excl = argument->dle.exclude_file->first; excl != NULL;
	     excl = excl->next) {
	    char *ex;
	    if (strcmp(excl->name, "./") == 0) {
		ex = g_strdup_printf("pat=%s", excl->name+2);
	    } else {
		ex = g_strdup_printf("pat=%s", excl->name);
	    }
	    g_ptr_array_add(argv_ptr, ex);
	}
    }
    if (argument->dle.exclude_list &&
	argument->dle.exclude_list->nb_element >= 1) {
	sle_t *excl;
	for (excl = argument->dle.exclude_list->first; excl != NULL;
	     excl = excl->next) {
	    char *exclname = fixup_relative(excl->name, argument->dle.device);
	    FILE *exclude;
	    char *aexc;
	    if ((exclude = fopen(exclname, "r")) != NULL) {
		while ((aexc = agets(exclude)) != NULL) {
		    if (aexc[0] != '\0') {
			char *ex;
			if (strcmp(aexc, "./") == 0) {
			    ex = g_strdup_printf("pat=%s", aexc+2);
			} else {
			    ex = g_strdup_printf("pat=%s", aexc);
			}
			g_ptr_array_add(argv_ptr, ex);
		    }
		    amfree(aexc);
		}
		fclose(exclude);
	    }
	    amfree(exclname);
	}
    }

    if (argument->dle.include_file &&
	argument->dle.include_file->nb_element >= 1) {
	sle_t *excl;
	for (excl = argument->dle.include_file->first; excl != NULL;
	     excl = excl->next) {
	    char *ex;
	    if (g_str_equal(excl->name, "./")) {
		ex = g_strdup_printf("pat=%s", excl->name+2);
	    } else {
		ex = g_strdup_printf("pat=%s", excl->name);
	    }
	    g_ptr_array_add(argv_ptr, ex);
	}
    }
    if (argument->dle.include_list &&
	argument->dle.include_list->nb_element >= 1) {
	sle_t *excl;
	for (excl = argument->dle.include_list->first; excl != NULL;
	     excl = excl->next) {
	    char *exclname = fixup_relative(excl->name, argument->dle.device);
	    FILE *include;
	    char *aexc;
	    if ((include = fopen(exclname, "r")) != NULL) {
		while ((aexc = agets(include)) != NULL) {
		    if (aexc[0] != '\0') {
			char *ex;
			if (g_str_equal(aexc, "./")) {
			    ex = g_strdup_printf("pat=%s", aexc+2);
			} else {
			    ex = g_strdup_printf("pat=%s", aexc);
			}
			g_ptr_array_add(argv_ptr, ex);
		    }
		    amfree(aexc);
		}
		fclose(include);
	    }
	    amfree(exclname);
	}
    }

    /* It is best to place command_options at the and of command line.
     * For example '-find' option requires that it is the last option used.
     * See: http://cdrecord.berlios.de/private/man/star/star.1.html
     */
    for (copt = argument->command_options; copt != NULL; copt = copt->next) {
	g_ptr_array_add(argv_ptr, stralloc((char *)copt->data));
    }

    g_ptr_array_add(argv_ptr, stralloc("."));

    g_ptr_array_add(argv_ptr, NULL);

    amfree(tardumpfile);
    amfree(fsname);

    return(argv_ptr);
}