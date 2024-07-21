parse_cmd_address(exarg_T *eap, char **errormsg, int silent)
{
    int		address_count = 1;
    linenr_T	lnum;

    // Repeat for all ',' or ';' separated addresses.
    for (;;)
    {
	eap->line1 = eap->line2;
	eap->line2 = default_address(eap);
	eap->cmd = skipwhite(eap->cmd);
	lnum = get_address(eap, &eap->cmd, eap->addr_type, eap->skip, silent,
					eap->addr_count == 0, address_count++);
	if (eap->cmd == NULL)	// error detected
	    return FAIL;
	if (lnum == MAXLNUM)
	{
	    if (*eap->cmd == '%')   // '%' - all lines
	    {
		++eap->cmd;
		switch (eap->addr_type)
		{
		    case ADDR_LINES:
		    case ADDR_OTHER:
			eap->line1 = 1;
			eap->line2 = curbuf->b_ml.ml_line_count;
			break;
		    case ADDR_LOADED_BUFFERS:
			{
			    buf_T	*buf = firstbuf;

			    while (buf->b_next != NULL
						  && buf->b_ml.ml_mfp == NULL)
				buf = buf->b_next;
			    eap->line1 = buf->b_fnum;
			    buf = lastbuf;
			    while (buf->b_prev != NULL
						  && buf->b_ml.ml_mfp == NULL)
				buf = buf->b_prev;
			    eap->line2 = buf->b_fnum;
			    break;
			}
		    case ADDR_BUFFERS:
			eap->line1 = firstbuf->b_fnum;
			eap->line2 = lastbuf->b_fnum;
			break;
		    case ADDR_WINDOWS:
		    case ADDR_TABS:
			if (IS_USER_CMDIDX(eap->cmdidx))
			{
			    eap->line1 = 1;
			    eap->line2 = eap->addr_type == ADDR_WINDOWS
						  ? LAST_WIN_NR : LAST_TAB_NR;
			}
			else
			{
			    // there is no Vim command which uses '%' and
			    // ADDR_WINDOWS or ADDR_TABS
			    *errormsg = _(e_invalid_range);
			    return FAIL;
			}
			break;
		    case ADDR_TABS_RELATIVE:
		    case ADDR_UNSIGNED:
		    case ADDR_QUICKFIX:
			*errormsg = _(e_invalid_range);
			return FAIL;
		    case ADDR_ARGUMENTS:
			if (ARGCOUNT == 0)
			    eap->line1 = eap->line2 = 0;
			else
			{
			    eap->line1 = 1;
			    eap->line2 = ARGCOUNT;
			}
			break;
		    case ADDR_QUICKFIX_VALID:
#ifdef FEAT_QUICKFIX
			eap->line1 = 1;
			eap->line2 = qf_get_valid_size(eap);
			if (eap->line2 == 0)
			    eap->line2 = 1;
#endif
			break;
		    case ADDR_NONE:
			// Will give an error later if a range is found.
			break;
		}
		++eap->addr_count;
	    }
	    else if (*eap->cmd == '*' && vim_strchr(p_cpo, CPO_STAR) == NULL)
	    {
		pos_T	    *fp;

		// '*' - visual area
		if (eap->addr_type != ADDR_LINES)
		{
		    *errormsg = _(e_invalid_range);
		    return FAIL;
		}

		++eap->cmd;
		if (!eap->skip)
		{
		    fp = getmark('<', FALSE);
		    if (check_mark(fp) == FAIL)
			return FAIL;
		    eap->line1 = fp->lnum;
		    fp = getmark('>', FALSE);
		    if (check_mark(fp) == FAIL)
			return FAIL;
		    eap->line2 = fp->lnum;
		    ++eap->addr_count;
		}
	    }
	}
	else
	    eap->line2 = lnum;
	eap->addr_count++;

	if (*eap->cmd == ';')
	{
	    if (!eap->skip)
	    {
		curwin->w_cursor.lnum = eap->line2;
		// Don't leave the cursor on an illegal line or column, but do
		// accept zero as address, so 0;/PATTERN/ works correctly.
		if (eap->line2 > 0)
		    check_cursor();
	    }
	}
	else if (*eap->cmd != ',')
	    break;
	++eap->cmd;
    }

    // One address given: set start and end lines.
    if (eap->addr_count == 1)
    {
	eap->line1 = eap->line2;
	// ... but only implicit: really no address given
	if (lnum == MAXLNUM)
	    eap->addr_count = 0;
    }
    return OK;
}