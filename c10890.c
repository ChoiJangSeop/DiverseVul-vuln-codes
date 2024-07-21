find_file_in_path_option(
    char_u	*ptr,		// file name
    int		len,		// length of file name
    int		options,
    int		first,		// use count'th matching file name
    char_u	*path_option,	// p_path or p_cdpath
    int		find_what,	// FINDFILE_FILE, _DIR or _BOTH
    char_u	*rel_fname,	// file name we are looking relative to.
    char_u	*suffixes)	// list of suffixes, 'suffixesadd' option
{
    static char_u	*dir;
    static int		did_findfile_init = FALSE;
    char_u		save_char;
    char_u		*file_name = NULL;
    char_u		*buf = NULL;
    int			rel_to_curdir;
# ifdef AMIGA
    struct Process	*proc = (struct Process *)FindTask(0L);
    APTR		save_winptr = proc->pr_WindowPtr;

    // Avoid a requester here for a volume that doesn't exist.
    proc->pr_WindowPtr = (APTR)-1L;
# endif

    if (first == TRUE)
    {
	// copy file name into NameBuff, expanding environment variables
	save_char = ptr[len];
	ptr[len] = NUL;
	expand_env_esc(ptr, NameBuff, MAXPATHL, FALSE, TRUE, NULL);
	ptr[len] = save_char;

	vim_free(ff_file_to_find);
	ff_file_to_find = vim_strsave(NameBuff);
	if (ff_file_to_find == NULL)	// out of memory
	{
	    file_name = NULL;
	    goto theend;
	}
	if (options & FNAME_UNESC)
	{
	    // Change all "\ " to " ".
	    for (ptr = ff_file_to_find; *ptr != NUL; ++ptr)
		if (ptr[0] == '\\' && ptr[1] == ' ')
		    mch_memmove(ptr, ptr + 1, STRLEN(ptr));
	}
    }

    rel_to_curdir = (ff_file_to_find[0] == '.'
		    && (ff_file_to_find[1] == NUL
			|| vim_ispathsep(ff_file_to_find[1])
			|| (ff_file_to_find[1] == '.'
			    && (ff_file_to_find[2] == NUL
				|| vim_ispathsep(ff_file_to_find[2])))));
    if (vim_isAbsName(ff_file_to_find)
	    // "..", "../path", "." and "./path": don't use the path_option
	    || rel_to_curdir
# if defined(MSWIN)
	    // handle "\tmp" as absolute path
	    || vim_ispathsep(ff_file_to_find[0])
	    // handle "c:name" as absolute path
	    || (ff_file_to_find[0] != NUL && ff_file_to_find[1] == ':')
# endif
# ifdef AMIGA
	    // handle ":tmp" as absolute path
	    || ff_file_to_find[0] == ':'
# endif
       )
    {
	/*
	 * Absolute path, no need to use "path_option".
	 * If this is not a first call, return NULL.  We already returned a
	 * filename on the first call.
	 */
	if (first == TRUE)
	{
	    int		l;
	    int		run;

	    if (path_with_url(ff_file_to_find))
	    {
		file_name = vim_strsave(ff_file_to_find);
		goto theend;
	    }

	    // When FNAME_REL flag given first use the directory of the file.
	    // Otherwise or when this fails use the current directory.
	    for (run = 1; run <= 2; ++run)
	    {
		l = (int)STRLEN(ff_file_to_find);
		if (run == 1
			&& rel_to_curdir
			&& (options & FNAME_REL)
			&& rel_fname != NULL
			&& STRLEN(rel_fname) + l < MAXPATHL)
		{
		    STRCPY(NameBuff, rel_fname);
		    STRCPY(gettail(NameBuff), ff_file_to_find);
		    l = (int)STRLEN(NameBuff);
		}
		else
		{
		    STRCPY(NameBuff, ff_file_to_find);
		    run = 2;
		}

		// When the file doesn't exist, try adding parts of
		// 'suffixesadd'.
		buf = suffixes;
		for (;;)
		{
		    if (mch_getperm(NameBuff) >= 0
			     && (find_what == FINDFILE_BOTH
				 || ((find_what == FINDFILE_DIR)
						    == mch_isdir(NameBuff))))
		    {
			file_name = vim_strsave(NameBuff);
			goto theend;
		    }
		    if (*buf == NUL)
			break;
		    copy_option_part(&buf, NameBuff + l, MAXPATHL - l, ",");
		}
	    }
	}
    }
    else
    {
	/*
	 * Loop over all paths in the 'path' or 'cdpath' option.
	 * When "first" is set, first setup to the start of the option.
	 * Otherwise continue to find the next match.
	 */
	if (first == TRUE)
	{
	    // vim_findfile_free_visited can handle a possible NULL pointer
	    vim_findfile_free_visited(fdip_search_ctx);
	    dir = path_option;
	    did_findfile_init = FALSE;
	}

	for (;;)
	{
	    if (did_findfile_init)
	    {
		file_name = vim_findfile(fdip_search_ctx);
		if (file_name != NULL)
		    break;

		did_findfile_init = FALSE;
	    }
	    else
	    {
		char_u  *r_ptr;

		if (dir == NULL || *dir == NUL)
		{
		    // We searched all paths of the option, now we can
		    // free the search context.
		    vim_findfile_cleanup(fdip_search_ctx);
		    fdip_search_ctx = NULL;
		    break;
		}

		if ((buf = alloc(MAXPATHL)) == NULL)
		    break;

		// copy next path
		buf[0] = 0;
		copy_option_part(&dir, buf, MAXPATHL, " ,");

# ifdef FEAT_PATH_EXTRA
		// get the stopdir string
		r_ptr = vim_findfile_stopdir(buf);
# else
		r_ptr = NULL;
# endif
		fdip_search_ctx = vim_findfile_init(buf, ff_file_to_find,
					    r_ptr, 100, FALSE, find_what,
					   fdip_search_ctx, FALSE, rel_fname);
		if (fdip_search_ctx != NULL)
		    did_findfile_init = TRUE;
		vim_free(buf);
	    }
	}
    }
    if (file_name == NULL && (options & FNAME_MESS))
    {
	if (first == TRUE)
	{
	    if (find_what == FINDFILE_DIR)
		semsg(_("E344: Can't find directory \"%s\" in cdpath"),
			ff_file_to_find);
	    else
		semsg(_("E345: Can't find file \"%s\" in path"),
			ff_file_to_find);
	}
	else
	{
	    if (find_what == FINDFILE_DIR)
		semsg(_("E346: No more directory \"%s\" found in cdpath"),
			ff_file_to_find);
	    else
		semsg(_("E347: No more file \"%s\" found in path"),
			ff_file_to_find);
	}
    }

theend:
# ifdef AMIGA
    proc->pr_WindowPtr = save_winptr;
# endif
    return file_name;
}