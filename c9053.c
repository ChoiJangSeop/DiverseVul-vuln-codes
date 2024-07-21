f_histadd(typval_T *argvars UNUSED, typval_T *rettv)
{
#ifdef FEAT_CMDHIST
    int		histype;
    char_u	*str;
    char_u	buf[NUMBUFLEN];
#endif

    rettv->vval.v_number = FALSE;
    if (check_restricted() || check_secure())
	return;
#ifdef FEAT_CMDHIST
    str = tv_get_string_chk(&argvars[0]);	/* NULL on type error */
    histype = str != NULL ? get_histtype(str) : -1;
    if (histype >= 0)
    {
	str = tv_get_string_buf(&argvars[1], buf);
	if (*str != NUL)
	{
	    init_history();
	    add_to_history(histype, str, FALSE, NUL);
	    rettv->vval.v_number = TRUE;
	    return;
	}
    }
#endif
}