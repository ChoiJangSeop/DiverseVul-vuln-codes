ins_compl_add(
    char_u	*str,
    int		len,
    char_u	*fname,
    char_u	**cptext,	    // extra text for popup menu or NULL
    typval_T	*user_data UNUSED,  // "user_data" entry or NULL
    int		cdir,
    int		flags_arg,
    int		adup)		// accept duplicate match
{
    compl_T	*match;
    int		dir = (cdir == 0 ? compl_direction : cdir);
    int		flags = flags_arg;

    if (flags & CP_FAST)
	fast_breakcheck();
    else
	ui_breakcheck();
    if (got_int)
	return FAIL;
    if (len < 0)
	len = (int)STRLEN(str);

    // If the same match is already present, don't add it.
    if (compl_first_match != NULL && !adup)
    {
	match = compl_first_match;
	do
	{
	    if (!match_at_original_text(match)
		    && STRNCMP(match->cp_str, str, len) == 0
		    && match->cp_str[len] == NUL)
		return NOTDONE;
	    match = match->cp_next;
	} while (match != NULL && !is_first_match(match));
    }

    // Remove any popup menu before changing the list of matches.
    ins_compl_del_pum();

    // Allocate a new match structure.
    // Copy the values to the new match structure.
    match = ALLOC_CLEAR_ONE(compl_T);
    if (match == NULL)
	return FAIL;
    match->cp_number = -1;
    if (flags & CP_ORIGINAL_TEXT)
	match->cp_number = 0;
    if ((match->cp_str = vim_strnsave(str, len)) == NULL)
    {
	vim_free(match);
	return FAIL;
    }

    // match-fname is:
    // - compl_curr_match->cp_fname if it is a string equal to fname.
    // - a copy of fname, CP_FREE_FNAME is set to free later THE allocated mem.
    // - NULL otherwise.	--Acevedo
    if (fname != NULL
	    && compl_curr_match != NULL
	    && compl_curr_match->cp_fname != NULL
	    && STRCMP(fname, compl_curr_match->cp_fname) == 0)
	match->cp_fname = compl_curr_match->cp_fname;
    else if (fname != NULL)
    {
	match->cp_fname = vim_strsave(fname);
	flags |= CP_FREE_FNAME;
    }
    else
	match->cp_fname = NULL;
    match->cp_flags = flags;

    if (cptext != NULL)
    {
	int i;

	for (i = 0; i < CPT_COUNT; ++i)
	    if (cptext[i] != NULL && *cptext[i] != NUL)
		match->cp_text[i] = vim_strsave(cptext[i]);
    }
#ifdef FEAT_EVAL
    if (user_data != NULL)
	match->cp_user_data = *user_data;
#endif

    // Link the new match structure after (FORWARD) or before (BACKWARD) the
    // current match in the list of matches .
    if (compl_first_match == NULL)
	match->cp_next = match->cp_prev = NULL;
    else if (dir == FORWARD)
    {
	match->cp_next = compl_curr_match->cp_next;
	match->cp_prev = compl_curr_match;
    }
    else	// BACKWARD
    {
	match->cp_next = compl_curr_match;
	match->cp_prev = compl_curr_match->cp_prev;
    }
    if (match->cp_next)
	match->cp_next->cp_prev = match;
    if (match->cp_prev)
	match->cp_prev->cp_next = match;
    else	// if there's nothing before, it is the first match
	compl_first_match = match;
    compl_curr_match = match;

    // Find the longest common string if still doing that.
    if (compl_get_longest && (flags & CP_ORIGINAL_TEXT) == 0)
	ins_compl_longest_match(match);

    return OK;
}