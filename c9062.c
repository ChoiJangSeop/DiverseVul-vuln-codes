f_setbufvar(typval_T *argvars, typval_T *rettv UNUSED)
{
    buf_T	*buf;
    char_u	*varname, *bufvarname;
    typval_T	*varp;
    char_u	nbuf[NUMBUFLEN];

    if (check_restricted() || check_secure())
	return;
    (void)tv_get_number(&argvars[0]);	    /* issue errmsg if type error */
    varname = tv_get_string_chk(&argvars[1]);
    buf = tv_get_buf(&argvars[0], FALSE);
    varp = &argvars[2];

    if (buf != NULL && varname != NULL && varp != NULL)
    {
	if (*varname == '&')
	{
	    long	numval;
	    char_u	*strval;
	    int		error = FALSE;
	    aco_save_T	aco;

	    /* set curbuf to be our buf, temporarily */
	    aucmd_prepbuf(&aco, buf);

	    ++varname;
	    numval = (long)tv_get_number_chk(varp, &error);
	    strval = tv_get_string_buf_chk(varp, nbuf);
	    if (!error && strval != NULL)
		set_option_value(varname, numval, strval, OPT_LOCAL);

	    /* reset notion of buffer */
	    aucmd_restbuf(&aco);
	}
	else
	{
	    buf_T *save_curbuf = curbuf;

	    bufvarname = alloc((unsigned)STRLEN(varname) + 3);
	    if (bufvarname != NULL)
	    {
		curbuf = buf;
		STRCPY(bufvarname, "b:");
		STRCPY(bufvarname + 2, varname);
		set_var(bufvarname, varp, TRUE);
		vim_free(bufvarname);
		curbuf = save_curbuf;
	    }
	}
    }
}