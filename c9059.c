f_writefile(typval_T *argvars, typval_T *rettv)
{
    int		binary = FALSE;
    int		append = FALSE;
#ifdef HAVE_FSYNC
    int		do_fsync = p_fs;
#endif
    char_u	*fname;
    FILE	*fd;
    int		ret = 0;
    listitem_T	*li;
    list_T	*list = NULL;
    blob_T	*blob = NULL;

    rettv->vval.v_number = -1;
    if (check_restricted() || check_secure())
	return;

    if (argvars[0].v_type == VAR_LIST)
    {
	list = argvars[0].vval.v_list;
	if (list == NULL)
	    return;
	for (li = list->lv_first; li != NULL; li = li->li_next)
	    if (tv_get_string_chk(&li->li_tv) == NULL)
		return;
    }
    else if (argvars[0].v_type == VAR_BLOB)
    {
	blob = argvars[0].vval.v_blob;
	if (blob == NULL)
	    return;
    }
    else
    {
	semsg(_(e_invarg2), "writefile()");
	return;
    }

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	char_u *arg2 = tv_get_string_chk(&argvars[2]);

	if (arg2 == NULL)
	    return;
	if (vim_strchr(arg2, 'b') != NULL)
	    binary = TRUE;
	if (vim_strchr(arg2, 'a') != NULL)
	    append = TRUE;
#ifdef HAVE_FSYNC
	if (vim_strchr(arg2, 's') != NULL)
	    do_fsync = TRUE;
	else if (vim_strchr(arg2, 'S') != NULL)
	    do_fsync = FALSE;
#endif
    }

    fname = tv_get_string_chk(&argvars[1]);
    if (fname == NULL)
	return;

    /* Always open the file in binary mode, library functions have a mind of
     * their own about CR-LF conversion. */
    if (*fname == NUL || (fd = mch_fopen((char *)fname,
				      append ? APPENDBIN : WRITEBIN)) == NULL)
    {
	semsg(_(e_notcreate), *fname == NUL ? (char_u *)_("<empty>") : fname);
	ret = -1;
    }
    else if (blob)
    {
	if (write_blob(fd, blob) == FAIL)
	    ret = -1;
#ifdef HAVE_FSYNC
	else if (do_fsync)
	    // Ignore the error, the user wouldn't know what to do about it.
	    // May happen for a device.
	    vim_ignored = fsync(fileno(fd));
#endif
	fclose(fd);
    }
    else
    {
	if (write_list(fd, list, binary) == FAIL)
	    ret = -1;
#ifdef HAVE_FSYNC
	else if (do_fsync)
	    /* Ignore the error, the user wouldn't know what to do about it.
	     * May happen for a device. */
	    vim_ignored = fsync(fileno(fd));
#endif
	fclose(fd);
    }

    rettv->vval.v_number = ret;
}