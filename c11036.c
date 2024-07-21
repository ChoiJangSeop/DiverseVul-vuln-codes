getvcol(
    win_T	*wp,
    pos_T	*pos,
    colnr_T	*start,
    colnr_T	*cursor,
    colnr_T	*end)
{
    colnr_T	vcol;
    char_u	*ptr;		// points to current char
    char_u	*posptr;	// points to char at pos->col
    char_u	*line;		// start of the line
    int		incr;
    int		head;
#ifdef FEAT_VARTABS
    int		*vts = wp->w_buffer->b_p_vts_array;
#endif
    int		ts = wp->w_buffer->b_p_ts;
    int		c;

    vcol = 0;
    line = ptr = ml_get_buf(wp->w_buffer, pos->lnum, FALSE);
    if (pos->col == MAXCOL)
	posptr = NULL;  // continue until the NUL
    else
    {
	// Special check for an empty line, which can happen on exit, when
	// ml_get_buf() always returns an empty string.
	if (*ptr == NUL)
	    pos->col = 0;
	posptr = ptr + pos->col;
	if (has_mbyte)
	    // always start on the first byte
	    posptr -= (*mb_head_off)(line, posptr);
    }

    /*
     * This function is used very often, do some speed optimizations.
     * When 'list', 'linebreak', 'showbreak' and 'breakindent' are not set
     * use a simple loop.
     * Also use this when 'list' is set but tabs take their normal size.
     */
    if ((!wp->w_p_list || wp->w_lcs_chars.tab1 != NUL)
#ifdef FEAT_LINEBREAK
	    && !wp->w_p_lbr && *get_showbreak_value(wp) == NUL && !wp->w_p_bri
#endif
       )
    {
	for (;;)
	{
	    head = 0;
	    c = *ptr;
	    // make sure we don't go past the end of the line
	    if (c == NUL)
	    {
		incr = 1;	// NUL at end of line only takes one column
		break;
	    }
	    // A tab gets expanded, depending on the current column
	    if (c == TAB)
#ifdef FEAT_VARTABS
		incr = tabstop_padding(vcol, ts, vts);
#else
		incr = ts - (vcol % ts);
#endif
	    else
	    {
		if (has_mbyte)
		{
		    // For utf-8, if the byte is >= 0x80, need to look at
		    // further bytes to find the cell width.
		    if (enc_utf8 && c >= 0x80)
			incr = utf_ptr2cells(ptr);
		    else
			incr = g_chartab[c] & CT_CELL_MASK;

		    // If a double-cell char doesn't fit at the end of a line
		    // it wraps to the next line, it's like this char is three
		    // cells wide.
		    if (incr == 2 && wp->w_p_wrap && MB_BYTE2LEN(*ptr) > 1
			    && in_win_border(wp, vcol))
		    {
			++incr;
			head = 1;
		    }
		}
		else
		    incr = g_chartab[c] & CT_CELL_MASK;
	    }

	    if (posptr != NULL && ptr >= posptr) // character at pos->col
		break;

	    vcol += incr;
	    MB_PTR_ADV(ptr);
	}
    }
    else
    {
	for (;;)
	{
	    // A tab gets expanded, depending on the current column
	    head = 0;
	    incr = win_lbr_chartabsize(wp, line, ptr, vcol, &head);
	    // make sure we don't go past the end of the line
	    if (*ptr == NUL)
	    {
		incr = 1;	// NUL at end of line only takes one column
		break;
	    }

	    if (posptr != NULL && ptr >= posptr) // character at pos->col
		break;

	    vcol += incr;
	    MB_PTR_ADV(ptr);
	}
    }
    if (start != NULL)
	*start = vcol + head;
    if (end != NULL)
	*end = vcol + incr - 1;
    if (cursor != NULL)
    {
	if (*ptr == TAB
		&& (State & NORMAL)
		&& !wp->w_p_list
		&& !virtual_active()
		&& !(VIsual_active
				&& (*p_sel == 'e' || LTOREQ_POS(*pos, VIsual)))
		)
	    *cursor = vcol + incr - 1;	    // cursor at end
	else
	    *cursor = vcol + head;	    // cursor at start
    }
}