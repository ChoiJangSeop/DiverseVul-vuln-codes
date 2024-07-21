f_settabvar(typval_T *argvars, typval_T *rettv)
{
    tabpage_T	*save_curtab;
    tabpage_T	*tp;
    char_u	*varname, *tabvarname;
    typval_T	*varp;

    rettv->vval.v_number = 0;

    if (check_restricted() || check_secure())
	return;

    tp = find_tabpage((int)tv_get_number_chk(&argvars[0], NULL));
    varname = tv_get_string_chk(&argvars[1]);
    varp = &argvars[2];

    if (varname != NULL && varp != NULL && tp != NULL)
    {
	save_curtab = curtab;
	goto_tabpage_tp(tp, FALSE, FALSE);

	tabvarname = alloc((unsigned)STRLEN(varname) + 3);
	if (tabvarname != NULL)
	{
	    STRCPY(tabvarname, "t:");
	    STRCPY(tabvarname + 2, varname);
	    set_var(tabvarname, varp, TRUE);
	    vim_free(tabvarname);
	}

	/* Restore current tabpage */
	if (valid_tabpage(save_curtab))
	    goto_tabpage_tp(save_curtab, FALSE, FALSE);
    }
}