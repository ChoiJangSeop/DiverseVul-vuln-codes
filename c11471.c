stop_insert(
    pos_T	*end_insert_pos,
    int		esc,			// called by ins_esc()
    int		nomove)			// <c-\><c-o>, don't move cursor
{
    int		cc;
    char_u	*ptr;

    stop_redo_ins();
    replace_flush();		// abandon replace stack

    /*
     * Save the inserted text for later redo with ^@ and CTRL-A.
     * Don't do it when "restart_edit" was set and nothing was inserted,
     * otherwise CTRL-O w and then <Left> will clear "last_insert".
     */
    ptr = get_inserted();
    if (did_restart_edit == 0 || (ptr != NULL
				       && (int)STRLEN(ptr) > new_insert_skip))
    {
	vim_free(last_insert);
	last_insert = ptr;
	last_insert_skip = new_insert_skip;
    }
    else
	vim_free(ptr);

    if (!arrow_used && end_insert_pos != NULL)
    {
	// Auto-format now.  It may seem strange to do this when stopping an
	// insertion (or moving the cursor), but it's required when appending
	// a line and having it end in a space.  But only do it when something
	// was actually inserted, otherwise undo won't work.
	if (!ins_need_undo && has_format_option(FO_AUTO))
	{
	    pos_T   tpos = curwin->w_cursor;

	    // When the cursor is at the end of the line after a space the
	    // formatting will move it to the following word.  Avoid that by
	    // moving the cursor onto the space.
	    cc = 'x';
	    if (curwin->w_cursor.col > 0 && gchar_cursor() == NUL)
	    {
		dec_cursor();
		cc = gchar_cursor();
		if (!VIM_ISWHITE(cc))
		    curwin->w_cursor = tpos;
	    }

	    auto_format(TRUE, FALSE);

	    if (VIM_ISWHITE(cc))
	    {
		if (gchar_cursor() != NUL)
		    inc_cursor();
		// If the cursor is still at the same character, also keep
		// the "coladd".
		if (gchar_cursor() == NUL
			&& curwin->w_cursor.lnum == tpos.lnum
			&& curwin->w_cursor.col == tpos.col)
		    curwin->w_cursor.coladd = tpos.coladd;
	    }
	}

	// If a space was inserted for auto-formatting, remove it now.
	check_auto_format(TRUE);

	// If we just did an auto-indent, remove the white space from the end
	// of the line, and put the cursor back.
	// Do this when ESC was used or moving the cursor up/down.
	// Check for the old position still being valid, just in case the text
	// got changed unexpectedly.
	if (!nomove && did_ai && (esc || (vim_strchr(p_cpo, CPO_INDENT) == NULL
			&& curwin->w_cursor.lnum != end_insert_pos->lnum))
		&& end_insert_pos->lnum <= curbuf->b_ml.ml_line_count)
	{
	    pos_T	tpos = curwin->w_cursor;

	    curwin->w_cursor = *end_insert_pos;
	    check_cursor_col();  // make sure it is not past the line
	    for (;;)
	    {
		if (gchar_cursor() == NUL && curwin->w_cursor.col > 0)
		    --curwin->w_cursor.col;
		cc = gchar_cursor();
		if (!VIM_ISWHITE(cc))
		    break;
		if (del_char(TRUE) == FAIL)
		    break;  // should not happen
	    }
	    if (curwin->w_cursor.lnum != tpos.lnum)
		curwin->w_cursor = tpos;
	    else
	    {
		// reset tpos, could have been invalidated in the loop above
		tpos = curwin->w_cursor;
		tpos.col++;
		if (cc != NUL && gchar_pos(&tpos) == NUL)
		    ++curwin->w_cursor.col;	// put cursor back on the NUL
	    }

	    // <C-S-Right> may have started Visual mode, adjust the position for
	    // deleted characters.
	    if (VIsual_active && VIsual.lnum == curwin->w_cursor.lnum)
	    {
		int len = (int)STRLEN(ml_get_curline());

		if (VIsual.col > len)
		{
		    VIsual.col = len;
		    VIsual.coladd = 0;
		}
	    }
	}
    }
    did_ai = FALSE;
#ifdef FEAT_SMARTINDENT
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
#endif

    // Set '[ and '] to the inserted text.  When end_insert_pos is NULL we are
    // now in a different buffer.
    if (end_insert_pos != NULL)
    {
	curbuf->b_op_start = Insstart;
	curbuf->b_op_start_orig = Insstart_orig;
	curbuf->b_op_end = *end_insert_pos;
    }
}