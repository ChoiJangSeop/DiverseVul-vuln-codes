nv_g_cmd(cmdarg_T *cap)
{
    oparg_T	*oap = cap->oap;
    int		i;

    switch (cap->nchar)
    {
    case Ctrl_A:
    case Ctrl_X:
#ifdef MEM_PROFILE
    // "g^A": dump log of used memory.
	if (!VIsual_active && cap->nchar == Ctrl_A)
	    vim_mem_profile_dump();
	else
#endif
    // "g^A/g^X": sequentially increment visually selected region
	     if (VIsual_active)
	{
	    cap->arg = TRUE;
	    cap->cmdchar = cap->nchar;
	    cap->nchar = NUL;
	    nv_addsub(cap);
	}
	else
	    clearopbeep(oap);
	break;

    // "gR": Enter virtual replace mode.
    case 'R':
	cap->arg = TRUE;
	nv_Replace(cap);
	break;

    case 'r':
	nv_vreplace(cap);
	break;

    case '&':
	do_cmdline_cmd((char_u *)"%s//~/&");
	break;

    // "gv": Reselect the previous Visual area.  If Visual already active,
    // exchange previous and current Visual area.
    case 'v':
	nv_gv_cmd(cap);
	break;

    // "gV": Don't reselect the previous Visual area after a Select mode
    // mapping of menu.
    case 'V':
	VIsual_reselect = FALSE;
	break;

    // "gh":  start Select mode.
    // "gH":  start Select line mode.
    // "g^H": start Select block mode.
    case K_BS:
	cap->nchar = Ctrl_H;
	// FALLTHROUGH
    case 'h':
    case 'H':
    case Ctrl_H:
	cap->cmdchar = cap->nchar + ('v' - 'h');
	cap->arg = TRUE;
	nv_visual(cap);
	break;

    // "gn", "gN" visually select next/previous search match
    // "gn" selects next match
    // "gN" selects previous match
    case 'N':
    case 'n':
	if (!current_search(cap->count1, cap->nchar == 'n'))
	    clearopbeep(oap);
	break;

    // "gj" and "gk" two new funny movement keys -- up and down
    // movement based on *screen* line rather than *file* line.
    case 'j':
    case K_DOWN:
	// with 'nowrap' it works just like the normal "j" command.
	if (!curwin->w_p_wrap)
	{
	    oap->motion_type = MLINE;
	    i = cursor_down(cap->count1, oap->op_type == OP_NOP);
	}
	else
	    i = nv_screengo(oap, FORWARD, cap->count1);
	if (i == FAIL)
	    clearopbeep(oap);
	break;

    case 'k':
    case K_UP:
	// with 'nowrap' it works just like the normal "k" command.
	if (!curwin->w_p_wrap)
	{
	    oap->motion_type = MLINE;
	    i = cursor_up(cap->count1, oap->op_type == OP_NOP);
	}
	else
	    i = nv_screengo(oap, BACKWARD, cap->count1);
	if (i == FAIL)
	    clearopbeep(oap);
	break;

    // "gJ": join two lines without inserting a space.
    case 'J':
	nv_join(cap);
	break;

    // "g0", "g^" : Like "0" and "^" but for screen lines.
    // "gm": middle of "g0" and "g$".
    case '^':
    case '0':
    case 'm':
    case K_HOME:
    case K_KHOME:
	nv_g_home_m_cmd(cap);
	break;

    case 'M':
	{
	    oap->motion_type = MCHAR;
	    oap->inclusive = FALSE;
	    i = linetabsize(ml_get_curline());
	    if (cap->count0 > 0 && cap->count0 <= 100)
		coladvance((colnr_T)(i * cap->count0 / 100));
	    else
		coladvance((colnr_T)(i / 2));
	    curwin->w_set_curswant = TRUE;
	}
	break;

    // "g_": to the last non-blank character in the line or <count> lines
    // downward.
    case '_':
	nv_g_underscore_cmd(cap);
	break;

    // "g$" : Like "$" but for screen lines.
    case '$':
    case K_END:
    case K_KEND:
	nv_g_dollar_cmd(cap);
	break;

    // "g*" and "g#", like "*" and "#" but without using "\<" and "\>"
    case '*':
    case '#':
#if POUND != '#'
    case POUND:		// pound sign (sometimes equal to '#')
#endif
    case Ctrl_RSB:		// :tag or :tselect for current identifier
    case ']':			// :tselect for current identifier
	nv_ident(cap);
	break;

    // ge and gE: go back to end of word
    case 'e':
    case 'E':
	oap->motion_type = MCHAR;
	curwin->w_set_curswant = TRUE;
	oap->inclusive = TRUE;
	if (bckend_word(cap->count1, cap->nchar == 'E', FALSE) == FAIL)
	    clearopbeep(oap);
	break;

    // "g CTRL-G": display info about cursor position
    case Ctrl_G:
	cursor_pos_info(NULL);
	break;

    // "gi": start Insert at the last position.
    case 'i':
	nv_gi_cmd(cap);
	break;

    // "gI": Start insert in column 1.
    case 'I':
	beginline(0);
	if (!checkclearopq(oap))
	    invoke_edit(cap, FALSE, 'g', FALSE);
	break;

#ifdef FEAT_SEARCHPATH
    // "gf": goto file, edit file under cursor
    // "]f" and "[f": can also be used.
    case 'f':
    case 'F':
	nv_gotofile(cap);
	break;
#endif

    // "g'm" and "g`m": jump to mark without setting pcmark
    case '\'':
	cap->arg = TRUE;
	// FALLTHROUGH
    case '`':
	nv_gomark(cap);
	break;

    // "gs": Goto sleep.
    case 's':
	do_sleep(cap->count1 * 1000L, FALSE);
	break;

    // "ga": Display the ascii value of the character under the
    // cursor.	It is displayed in decimal, hex, and octal. -- webb
    case 'a':
	do_ascii(NULL);
	break;

    // "g8": Display the bytes used for the UTF-8 character under the
    // cursor.	It is displayed in hex.
    // "8g8" finds illegal byte sequence.
    case '8':
	if (cap->count0 == 8)
	    utf_find_illegal();
	else
	    show_utf8();
	break;

    // "g<": show scrollback text
    case '<':
	show_sb_text();
	break;

    // "gg": Goto the first line in file.  With a count it goes to
    // that line number like for "G". -- webb
    case 'g':
	cap->arg = FALSE;
	nv_goto(cap);
	break;

    //	 Two-character operators:
    //	 "gq"	    Format text
    //	 "gw"	    Format text and keep cursor position
    //	 "g~"	    Toggle the case of the text.
    //	 "gu"	    Change text to lower case.
    //	 "gU"	    Change text to upper case.
    //   "g?"	    rot13 encoding
    //   "g@"	    call 'operatorfunc'
    case 'q':
    case 'w':
	oap->cursor_start = curwin->w_cursor;
	// FALLTHROUGH
    case '~':
    case 'u':
    case 'U':
    case '?':
    case '@':
	nv_operator(cap);
	break;

    // "gd": Find first occurrence of pattern under the cursor in the
    //	 current function
    // "gD": idem, but in the current file.
    case 'd':
    case 'D':
	nv_gd(oap, cap->nchar, (int)cap->count0);
	break;

    // g<*Mouse> : <C-*mouse>
    case K_MIDDLEMOUSE:
    case K_MIDDLEDRAG:
    case K_MIDDLERELEASE:
    case K_LEFTMOUSE:
    case K_LEFTDRAG:
    case K_LEFTRELEASE:
    case K_MOUSEMOVE:
    case K_RIGHTMOUSE:
    case K_RIGHTDRAG:
    case K_RIGHTRELEASE:
    case K_X1MOUSE:
    case K_X1DRAG:
    case K_X1RELEASE:
    case K_X2MOUSE:
    case K_X2DRAG:
    case K_X2RELEASE:
	mod_mask = MOD_MASK_CTRL;
	(void)do_mouse(oap, cap->nchar, BACKWARD, cap->count1, 0);
	break;

    case K_IGNORE:
	break;

    // "gP" and "gp": same as "P" and "p" but leave cursor just after new text
    case 'p':
    case 'P':
	nv_put(cap);
	break;

#ifdef FEAT_BYTEOFF
    // "go": goto byte count from start of buffer
    case 'o':
	goto_byte(cap->count0);
	break;
#endif

    // "gQ": improved Ex mode
    case 'Q':
	if (text_locked())
	{
	    clearopbeep(cap->oap);
	    text_locked_msg();
	    break;
	}

	if (!checkclearopq(oap))
	    do_exmode(TRUE);
	break;

    case ',':
	nv_pcmark(cap);
	break;

    case ';':
	cap->count1 = -cap->count1;
	nv_pcmark(cap);
	break;

    case 't':
	if (!checkclearop(oap))
	    goto_tabpage((int)cap->count0);
	break;
    case 'T':
	if (!checkclearop(oap))
	    goto_tabpage(-(int)cap->count1);
	break;

    case TAB:
	if (!checkclearop(oap) && goto_tabpage_lastused() == FAIL)
	    clearopbeep(oap);
	break;

    case '+':
    case '-': // "g+" and "g-": undo or redo along the timeline
	if (!checkclearopq(oap))
	    undo_time(cap->nchar == '-' ? -cap->count1 : cap->count1,
							 FALSE, FALSE, FALSE);
	break;

    default:
	clearopbeep(oap);
	break;
    }
}