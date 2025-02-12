ins_compl_get_exp(pos_T *ini)
{
    static ins_compl_next_state_T st;
    int		i;
    int		found_new_match;
    int		type = ctrl_x_mode;

    if (!compl_started)
    {
	FOR_ALL_BUFFERS(st.ins_buf)
	    st.ins_buf->b_scanned = 0;
	st.found_all = FALSE;
	st.ins_buf = curbuf;
	st.e_cpt = (compl_cont_status & CONT_LOCAL)
					    ? (char_u *)"." : curbuf->b_p_cpt;
	st.last_match_pos = st.first_match_pos = *ini;
    }
    else if (st.ins_buf != curbuf && !buf_valid(st.ins_buf))
	st.ins_buf = curbuf;  // In case the buffer was wiped out.

    compl_old_match = compl_curr_match;	// remember the last current match
    st.cur_match_pos = (compl_dir_forward())
				? &st.last_match_pos : &st.first_match_pos;

    // For ^N/^P loop over all the flags/windows/buffers in 'complete'.
    for (;;)
    {
	found_new_match = FAIL;
	st.set_match_pos = FALSE;

	// For ^N/^P pick a new entry from e_cpt if compl_started is off,
	// or if found_all says this entry is done.  For ^X^L only use the
	// entries from 'complete' that look in loaded buffers.
	if ((ctrl_x_mode_normal() || ctrl_x_mode_line_or_eval())
					&& (!compl_started || st.found_all))
	{
	    int status = process_next_cpt_value(&st, &type, ini);

	    if (status == INS_COMPL_CPT_END)
		break;
	    if (status == INS_COMPL_CPT_CONT)
		continue;
	}

	// If complete() was called then compl_pattern has been reset.  The
	// following won't work then, bail out.
	if (compl_pattern == NULL)
	    break;

	// get the next set of completion matches
	found_new_match = get_next_completion_match(type, &st, ini);

	// break the loop for specialized modes (use 'complete' just for the
	// generic ctrl_x_mode == CTRL_X_NORMAL) or when we've found a new
	// match
	if ((ctrl_x_mode_not_default() && !ctrl_x_mode_line_or_eval())
						|| found_new_match != FAIL)
	{
	    if (got_int)
		break;
	    // Fill the popup menu as soon as possible.
	    if (type != -1)
		ins_compl_check_keys(0, FALSE);

	    if ((ctrl_x_mode_not_default()
			&& !ctrl_x_mode_line_or_eval()) || compl_interrupted)
		break;
	    compl_started = TRUE;
	}
	else
	{
	    // Mark a buffer scanned when it has been scanned completely
	    if (type == 0 || type == CTRL_X_PATH_PATTERNS)
		st.ins_buf->b_scanned = TRUE;

	    compl_started = FALSE;
	}
    }
    compl_started = TRUE;

    if ((ctrl_x_mode_normal() || ctrl_x_mode_line_or_eval())
	    && *st.e_cpt == NUL)		// Got to end of 'complete'
	found_new_match = FAIL;

    i = -1;		// total of matches, unknown
    if (found_new_match == FAIL || (ctrl_x_mode_not_default()
					       && !ctrl_x_mode_line_or_eval()))
	i = ins_compl_make_cyclic();

    if (compl_old_match != NULL)
    {
	// If several matches were added (FORWARD) or the search failed and has
	// just been made cyclic then we have to move compl_curr_match to the
	// next or previous entry (if any) -- Acevedo
	compl_curr_match = compl_dir_forward() ? compl_old_match->cp_next
						    : compl_old_match->cp_prev;
	if (compl_curr_match == NULL)
	    compl_curr_match = compl_old_match;
    }
    may_trigger_modechanged();

    return i;
}