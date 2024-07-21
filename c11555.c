spell_move_to(
    win_T	*wp,
    int		dir,		// FORWARD or BACKWARD
    int		allwords,	// TRUE for "[s"/"]s", FALSE for "[S"/"]S"
    int		curline,
    hlf_T	*attrp)		// return: attributes of bad word or NULL
				// (only when "dir" is FORWARD)
{
    linenr_T	lnum;
    pos_T	found_pos;
    int		found_len = 0;
    char_u	*line;
    char_u	*p;
    char_u	*endp;
    hlf_T	attr;
    int		len;
#ifdef FEAT_SYN_HL
    int		has_syntax = syntax_present(wp);
#endif
    int		col;
    int		can_spell;
    char_u	*buf = NULL;
    int		buflen = 0;
    int		skip = 0;
    int		capcol = -1;
    int		found_one = FALSE;
    int		wrapped = FALSE;

    if (no_spell_checking(wp))
	return 0;

    /*
     * Start looking for bad word at the start of the line, because we can't
     * start halfway a word, we don't know where it starts or ends.
     *
     * When searching backwards, we continue in the line to find the last
     * bad word (in the cursor line: before the cursor).
     *
     * We concatenate the start of the next line, so that wrapped words work
     * (e.g. "et<line-break>cetera").  Doesn't work when searching backwards
     * though...
     */
    lnum = wp->w_cursor.lnum;
    CLEAR_POS(&found_pos);

    while (!got_int)
    {
	line = ml_get_buf(wp->w_buffer, lnum, FALSE);

	len = (int)STRLEN(line);
	if (buflen < len + MAXWLEN + 2)
	{
	    vim_free(buf);
	    buflen = len + MAXWLEN + 2;
	    buf = alloc(buflen);
	    if (buf == NULL)
		break;
	}

	// In first line check first word for Capital.
	if (lnum == 1)
	    capcol = 0;

	// For checking first word with a capital skip white space.
	if (capcol == 0)
	    capcol = getwhitecols(line);
	else if (curline && wp == curwin)
	{
	    // For spellbadword(): check if first word needs a capital.
	    col = getwhitecols(line);
	    if (check_need_cap(lnum, col))
		capcol = col;

	    // Need to get the line again, may have looked at the previous
	    // one.
	    line = ml_get_buf(wp->w_buffer, lnum, FALSE);
	}

	// Copy the line into "buf" and append the start of the next line if
	// possible.
	STRCPY(buf, line);
	if (lnum < wp->w_buffer->b_ml.ml_line_count)
	    spell_cat_line(buf + STRLEN(buf),
			  ml_get_buf(wp->w_buffer, lnum + 1, FALSE), MAXWLEN);

	p = buf + skip;
	endp = buf + len;
	while (p < endp)
	{
	    // When searching backward don't search after the cursor.  Unless
	    // we wrapped around the end of the buffer.
	    if (dir == BACKWARD
		    && lnum == wp->w_cursor.lnum
		    && !wrapped
		    && (colnr_T)(p - buf) >= wp->w_cursor.col)
		break;

	    // start of word
	    attr = HLF_COUNT;
	    len = spell_check(wp, p, &attr, &capcol, FALSE);

	    if (attr != HLF_COUNT)
	    {
		// We found a bad word.  Check the attribute.
		if (allwords || attr == HLF_SPB)
		{
		    // When searching forward only accept a bad word after
		    // the cursor.
		    if (dir == BACKWARD
			    || lnum != wp->w_cursor.lnum
			    || (wrapped
				|| (colnr_T)(curline ? p - buf + len
						     : p - buf)
						  > wp->w_cursor.col))
		    {
#ifdef FEAT_SYN_HL
			if (has_syntax)
			{
			    col = (int)(p - buf);
			    (void)syn_get_id(wp, lnum, (colnr_T)col,
						    FALSE, &can_spell, FALSE);
			    if (!can_spell)
				attr = HLF_COUNT;
			}
			else
#endif
			    can_spell = TRUE;

			if (can_spell)
			{
			    found_one = TRUE;
			    found_pos.lnum = lnum;
			    found_pos.col = (int)(p - buf);
			    found_pos.coladd = 0;
			    if (dir == FORWARD)
			    {
				// No need to search further.
				wp->w_cursor = found_pos;
				vim_free(buf);
				if (attrp != NULL)
				    *attrp = attr;
				return len;
			    }
			    else if (curline)
				// Insert mode completion: put cursor after
				// the bad word.
				found_pos.col += len;
			    found_len = len;
			}
		    }
		    else
			found_one = TRUE;
		}
	    }

	    // advance to character after the word
	    p += len;
	    capcol -= len;
	}

	if (dir == BACKWARD && found_pos.lnum != 0)
	{
	    // Use the last match in the line (before the cursor).
	    wp->w_cursor = found_pos;
	    vim_free(buf);
	    return found_len;
	}

	if (curline)
	    break;	// only check cursor line

	// If we are back at the starting line and searched it again there
	// is no match, give up.
	if (lnum == wp->w_cursor.lnum && wrapped)
	    break;

	// Advance to next line.
	if (dir == BACKWARD)
	{
	    if (lnum > 1)
		--lnum;
	    else if (!p_ws)
		break;	    // at first line and 'nowrapscan'
	    else
	    {
		// Wrap around to the end of the buffer.  May search the
		// starting line again and accept the last match.
		lnum = wp->w_buffer->b_ml.ml_line_count;
		wrapped = TRUE;
		if (!shortmess(SHM_SEARCH))
		    give_warning((char_u *)_(top_bot_msg), TRUE);
	    }
	    capcol = -1;
	}
	else
	{
	    if (lnum < wp->w_buffer->b_ml.ml_line_count)
		++lnum;
	    else if (!p_ws)
		break;	    // at first line and 'nowrapscan'
	    else
	    {
		// Wrap around to the start of the buffer.  May search the
		// starting line again and accept the first match.
		lnum = 1;
		wrapped = TRUE;
		if (!shortmess(SHM_SEARCH))
		    give_warning((char_u *)_(bot_top_msg), TRUE);
	    }

	    // If we are back at the starting line and there is no match then
	    // give up.
	    if (lnum == wp->w_cursor.lnum && !found_one)
		break;

	    // Skip the characters at the start of the next line that were
	    // included in a match crossing line boundaries.
	    if (attr == HLF_COUNT)
		skip = (int)(p - endp);
	    else
		skip = 0;

	    // Capcol skips over the inserted space.
	    --capcol;

	    // But after empty line check first word in next line
	    if (*skipwhite(line) == NUL)
		capcol = 0;
	}

	line_breakcheck();
    }

    vim_free(buf);
    return 0;
}