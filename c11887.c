process_next_cpt_value(
	ins_compl_next_state_T *st,
	int		*compl_type_arg,
	pos_T		*start_match_pos)
{
    int	    compl_type = -1;
    int	    status = INS_COMPL_CPT_OK;

    st->found_all = FALSE;

    while (*st->e_cpt == ',' || *st->e_cpt == ' ')
	st->e_cpt++;

    if (*st->e_cpt == '.' && !curbuf->b_scanned)
    {
	st->ins_buf = curbuf;
	st->first_match_pos = *start_match_pos;
	// Move the cursor back one character so that ^N can match the
	// word immediately after the cursor.
	if (ctrl_x_mode_normal() && dec(&st->first_match_pos) < 0)
	{
	    // Move the cursor to after the last character in the
	    // buffer, so that word at start of buffer is found
	    // correctly.
	    st->first_match_pos.lnum = st->ins_buf->b_ml.ml_line_count;
	    st->first_match_pos.col =
		(colnr_T)STRLEN(ml_get(st->first_match_pos.lnum));
	}
	st->last_match_pos = st->first_match_pos;
	compl_type = 0;

	// Remember the first match so that the loop stops when we
	// wrap and come back there a second time.
	st->set_match_pos = TRUE;
    }
    else if (vim_strchr((char_u *)"buwU", *st->e_cpt) != NULL
	    && (st->ins_buf = ins_compl_next_buf(st->ins_buf, *st->e_cpt)) != curbuf)
    {
	// Scan a buffer, but not the current one.
	if (st->ins_buf->b_ml.ml_mfp != NULL)   // loaded buffer
	{
	    compl_started = TRUE;
	    st->first_match_pos.col = st->last_match_pos.col = 0;
	    st->first_match_pos.lnum = st->ins_buf->b_ml.ml_line_count + 1;
	    st->last_match_pos.lnum = 0;
	    compl_type = 0;
	}
	else	// unloaded buffer, scan like dictionary
	{
	    st->found_all = TRUE;
	    if (st->ins_buf->b_fname == NULL)
	    {
		status = INS_COMPL_CPT_CONT;
		goto done;
	    }
	    compl_type = CTRL_X_DICTIONARY;
	    st->dict = st->ins_buf->b_fname;
	    st->dict_f = DICT_EXACT;
	}
	msg_hist_off = TRUE;	// reset in msg_trunc_attr()
	vim_snprintf((char *)IObuff, IOSIZE, _("Scanning: %s"),
		st->ins_buf->b_fname == NULL
		    ? buf_spname(st->ins_buf)
		    : st->ins_buf->b_sfname == NULL
			? st->ins_buf->b_fname
			: st->ins_buf->b_sfname);
	(void)msg_trunc_attr((char *)IObuff, TRUE, HL_ATTR(HLF_R));
    }
    else if (*st->e_cpt == NUL)
	status = INS_COMPL_CPT_END;
    else
    {
	if (ctrl_x_mode_line_or_eval())
	    compl_type = -1;
	else if (*st->e_cpt == 'k' || *st->e_cpt == 's')
	{
	    if (*st->e_cpt == 'k')
		compl_type = CTRL_X_DICTIONARY;
	    else
		compl_type = CTRL_X_THESAURUS;
	    if (*++st->e_cpt != ',' && *st->e_cpt != NUL)
	    {
		st->dict = st->e_cpt;
		st->dict_f = DICT_FIRST;
	    }
	}
#ifdef FEAT_FIND_ID
	else if (*st->e_cpt == 'i')
	    compl_type = CTRL_X_PATH_PATTERNS;
	else if (*st->e_cpt == 'd')
	    compl_type = CTRL_X_PATH_DEFINES;
#endif
	else if (*st->e_cpt == ']' || *st->e_cpt == 't')
	{
	    msg_hist_off = TRUE;	// reset in msg_trunc_attr()
	    compl_type = CTRL_X_TAGS;
	    vim_snprintf((char *)IObuff, IOSIZE, _("Scanning tags."));
	    (void)msg_trunc_attr((char *)IObuff, TRUE, HL_ATTR(HLF_R));
	}
	else
	    compl_type = -1;

	// in any case e_cpt is advanced to the next entry
	(void)copy_option_part(&st->e_cpt, IObuff, IOSIZE, ",");

	st->found_all = TRUE;
	if (compl_type == -1)
	    status = INS_COMPL_CPT_CONT;
    }

done:
    *compl_type_arg = compl_type;
    return status;
}