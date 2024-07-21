get_user_var_name(expand_T *xp, int idx)
{
    static long_u	gdone;
    static long_u	bdone;
    static long_u	wdone;
    static long_u	tdone;
    static int		vidx;
    static hashitem_T	*hi;
    hashtab_T		*ht;

    if (idx == 0)
    {
	gdone = bdone = wdone = vidx = 0;
	tdone = 0;
    }

    // Global variables
    if (gdone < globvarht.ht_used)
    {
	if (gdone++ == 0)
	    hi = globvarht.ht_array;
	else
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	if (STRNCMP("g:", xp->xp_pattern, 2) == 0)
	    return cat_prefix_varname('g', hi->hi_key);
	return hi->hi_key;
    }

    // b: variables
    ht =
#ifdef FEAT_CMDWIN
	// In cmdwin, the alternative buffer should be used.
	is_in_cmdwin() ? &prevwin->w_buffer->b_vars->dv_hashtab :
#endif
	&curbuf->b_vars->dv_hashtab;
    if (bdone < ht->ht_used)
    {
	if (bdone++ == 0)
	    hi = ht->ht_array;
	else
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	return cat_prefix_varname('b', hi->hi_key);
    }

    // w: variables
    ht =
#ifdef FEAT_CMDWIN
	// In cmdwin, the alternative window should be used.
	is_in_cmdwin() ? &prevwin->w_vars->dv_hashtab :
#endif
	&curwin->w_vars->dv_hashtab;
    if (wdone < ht->ht_used)
    {
	if (wdone++ == 0)
	    hi = ht->ht_array;
	else
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	return cat_prefix_varname('w', hi->hi_key);
    }

    // t: variables
    ht = &curtab->tp_vars->dv_hashtab;
    if (tdone < ht->ht_used)
    {
	if (tdone++ == 0)
	    hi = ht->ht_array;
	else
	    ++hi;
	while (HASHITEM_EMPTY(hi))
	    ++hi;
	return cat_prefix_varname('t', hi->hi_key);
    }

    // v: variables
    if (vidx < VV_LEN)
	return cat_prefix_varname('v', (char_u *)vimvars[vidx++].vv_name);

    VIM_CLEAR(varnamebuf);
    varnamebuflen = 0;
    return NULL;
}