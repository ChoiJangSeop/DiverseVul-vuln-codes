uc_list(char_u *name, size_t name_len)
{
    int		i, j;
    int		found = FALSE;
    ucmd_T	*cmd;
    int		len;
    int		over;
    long	a;
    garray_T	*gap;

    // In cmdwin, the alternative buffer should be used.
    gap =
#ifdef FEAT_CMDWIN
	    is_in_cmdwin() ? &prevwin->w_buffer->b_ucmds :
#endif
	    &curbuf->b_ucmds;
    for (;;)
    {
	for (i = 0; i < gap->ga_len; ++i)
	{
	    cmd = USER_CMD_GA(gap, i);
	    a = (long)cmd->uc_argt;

	    // Skip commands which don't match the requested prefix and
	    // commands filtered out.
	    if (STRNCMP(name, cmd->uc_name, name_len) != 0
		    || message_filtered(cmd->uc_name))
		continue;

	    // Put out the title first time
	    if (!found)
		msg_puts_title(_("\n    Name              Args Address Complete    Definition"));
	    found = TRUE;
	    msg_putchar('\n');
	    if (got_int)
		break;

	    // Special cases
	    len = 4;
	    if (a & EX_BANG)
	    {
		msg_putchar('!');
		--len;
	    }
	    if (a & EX_REGSTR)
	    {
		msg_putchar('"');
		--len;
	    }
	    if (gap != &ucmds)
	    {
		msg_putchar('b');
		--len;
	    }
	    if (a & EX_TRLBAR)
	    {
		msg_putchar('|');
		--len;
	    }
	    while (len-- > 0)
		msg_putchar(' ');

	    msg_outtrans_attr(cmd->uc_name, HL_ATTR(HLF_D));
	    len = (int)STRLEN(cmd->uc_name) + 4;

	    do {
		msg_putchar(' ');
		++len;
	    } while (len < 22);

	    // "over" is how much longer the name is than the column width for
	    // the name, we'll try to align what comes after.
	    over = len - 22;
	    len = 0;

	    // Arguments
	    switch ((int)(a & (EX_EXTRA|EX_NOSPC|EX_NEEDARG)))
	    {
		case 0:				IObuff[len++] = '0'; break;
		case (EX_EXTRA):		IObuff[len++] = '*'; break;
		case (EX_EXTRA|EX_NOSPC):	IObuff[len++] = '?'; break;
		case (EX_EXTRA|EX_NEEDARG):	IObuff[len++] = '+'; break;
		case (EX_EXTRA|EX_NOSPC|EX_NEEDARG): IObuff[len++] = '1'; break;
	    }

	    do {
		IObuff[len++] = ' ';
	    } while (len < 5 - over);

	    // Address / Range
	    if (a & (EX_RANGE|EX_COUNT))
	    {
		if (a & EX_COUNT)
		{
		    // -count=N
		    sprintf((char *)IObuff + len, "%ldc", cmd->uc_def);
		    len += (int)STRLEN(IObuff + len);
		}
		else if (a & EX_DFLALL)
		    IObuff[len++] = '%';
		else if (cmd->uc_def >= 0)
		{
		    // -range=N
		    sprintf((char *)IObuff + len, "%ld", cmd->uc_def);
		    len += (int)STRLEN(IObuff + len);
		}
		else
		    IObuff[len++] = '.';
	    }

	    do {
		IObuff[len++] = ' ';
	    } while (len < 8 - over);

	    // Address Type
	    for (j = 0; addr_type_complete[j].expand != ADDR_NONE; ++j)
		if (addr_type_complete[j].expand != ADDR_LINES
			&& addr_type_complete[j].expand == cmd->uc_addr_type)
		{
		    STRCPY(IObuff + len, addr_type_complete[j].shortname);
		    len += (int)STRLEN(IObuff + len);
		    break;
		}

	    do {
		IObuff[len++] = ' ';
	    } while (len < 13 - over);

	    // Completion
	    for (j = 0; command_complete[j].expand != 0; ++j)
		if (command_complete[j].expand == cmd->uc_compl)
		{
		    STRCPY(IObuff + len, command_complete[j].name);
		    len += (int)STRLEN(IObuff + len);
#ifdef FEAT_EVAL
		    if (p_verbose > 0 && cmd->uc_compl_arg != NULL
					    && STRLEN(cmd->uc_compl_arg) < 200)
		    {
			IObuff[len] = ',';
			STRCPY(IObuff + len + 1, cmd->uc_compl_arg);
			len += (int)STRLEN(IObuff + len);
		    }
#endif
		    break;
		}

	    do {
		IObuff[len++] = ' ';
	    } while (len < 25 - over);

	    IObuff[len] = '\0';
	    msg_outtrans(IObuff);

	    msg_outtrans_special(cmd->uc_rep, FALSE,
					     name_len == 0 ? Columns - 47 : 0);
#ifdef FEAT_EVAL
	    if (p_verbose > 0)
		last_set_msg(cmd->uc_script_ctx);
#endif
	    out_flush();
	    ui_breakcheck();
	    if (got_int)
		break;
	}
	if (gap == &ucmds || i < gap->ga_len)
	    break;
	gap = &ucmds;
    }

    if (!found)
	msg(_("No user-defined commands found"));
}