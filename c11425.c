cmdline_erase_chars(
	int c,
	int indent
#ifdef FEAT_SEARCH_EXTRA
	, incsearch_state_T *isp
#endif
	)
{
    int		i;
    int		j;

    if (c == K_KDEL)
	c = K_DEL;

    /*
     * Delete current character is the same as backspace on next
     * character, except at end of line.
     */
    if (c == K_DEL && ccline.cmdpos != ccline.cmdlen)
	++ccline.cmdpos;
    if (has_mbyte && c == K_DEL)
	ccline.cmdpos += mb_off_next(ccline.cmdbuff,
		ccline.cmdbuff + ccline.cmdpos);
    if (ccline.cmdpos > 0)
    {
	char_u *p;

	j = ccline.cmdpos;
	p = ccline.cmdbuff + j;
	if (has_mbyte)
	{
	    p = mb_prevptr(ccline.cmdbuff, p);
	    if (c == Ctrl_W)
	    {
		while (p > ccline.cmdbuff && vim_isspace(*p))
		    p = mb_prevptr(ccline.cmdbuff, p);
		i = mb_get_class(p);
		while (p > ccline.cmdbuff && mb_get_class(p) == i)
		    p = mb_prevptr(ccline.cmdbuff, p);
		if (mb_get_class(p) != i)
		    p += (*mb_ptr2len)(p);
	    }
	}
	else if (c == Ctrl_W)
	{
	    while (p > ccline.cmdbuff && vim_isspace(p[-1]))
		--p;
	    i = vim_iswordc(p[-1]);
	    while (p > ccline.cmdbuff && !vim_isspace(p[-1])
		    && vim_iswordc(p[-1]) == i)
		--p;
	}
	else
	    --p;
	ccline.cmdpos = (int)(p - ccline.cmdbuff);
	ccline.cmdlen -= j - ccline.cmdpos;
	i = ccline.cmdpos;
	while (i < ccline.cmdlen)
	    ccline.cmdbuff[i++] = ccline.cmdbuff[j++];

	// Truncate at the end, required for multi-byte chars.
	ccline.cmdbuff[ccline.cmdlen] = NUL;
#ifdef FEAT_SEARCH_EXTRA
	if (ccline.cmdlen == 0)
	{
	    isp->search_start = isp->save_cursor;
	    // save view settings, so that the screen
	    // won't be restored at the wrong position
	    isp->old_viewstate = isp->init_viewstate;
	}
#endif
	redrawcmd();
    }
    else if (ccline.cmdlen == 0 && c != Ctrl_W
	    && ccline.cmdprompt == NULL && indent == 0)
    {
	// In ex and debug mode it doesn't make sense to return.
	if (exmode_active
#ifdef FEAT_EVAL
		|| ccline.cmdfirstc == '>'
#endif
	   )
	    return CMDLINE_NOT_CHANGED;

	VIM_CLEAR(ccline.cmdbuff);	// no commandline to return
	if (!cmd_silent)
	{
#ifdef FEAT_RIGHTLEFT
	    if (cmdmsg_rl)
		msg_col = Columns;
	    else
#endif
		msg_col = 0;
	    msg_putchar(' ');		// delete ':'
	}
#ifdef FEAT_SEARCH_EXTRA
	if (ccline.cmdlen == 0)
	    isp->search_start = isp->save_cursor;
#endif
	redraw_cmdline = TRUE;
	return GOTO_NORMAL_MODE;
    }
    return CMDLINE_CHANGED;
}