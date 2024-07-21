do_window(
    int		nchar,
    long	Prenum,
    int		xchar)	    // extra char from ":wincmd gx" or NUL
{
    long	Prenum1;
    win_T	*wp;
#if defined(FEAT_SEARCHPATH) || defined(FEAT_FIND_ID)
    char_u	*ptr;
    linenr_T    lnum = -1;
#endif
#ifdef FEAT_FIND_ID
    int		type = FIND_DEFINE;
    int		len;
#endif
    char_u	cbuf[40];

    if (ERROR_IF_ANY_POPUP_WINDOW)
	return;

#ifdef FEAT_CMDWIN
# define CHECK_CMDWIN \
    do { \
	if (cmdwin_type != 0) \
	{ \
	    emsg(_(e_invalid_in_cmdline_window)); \
	    return; \
	} \
    } while (0)
#else
# define CHECK_CMDWIN do { /**/ } while (0)
#endif

    Prenum1 = Prenum == 0 ? 1 : Prenum;

    switch (nchar)
    {
// split current window in two parts, horizontally
    case 'S':
    case Ctrl_S:
    case 's':
		CHECK_CMDWIN;
		reset_VIsual_and_resel();	// stop Visual mode
#ifdef FEAT_QUICKFIX
		// When splitting the quickfix window open a new buffer in it,
		// don't replicate the quickfix buffer.
		if (bt_quickfix(curbuf))
		    goto newwindow;
#endif
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		(void)win_split((int)Prenum, 0);
		break;

// split current window in two parts, vertically
    case Ctrl_V:
    case 'v':
		CHECK_CMDWIN;
		reset_VIsual_and_resel();	// stop Visual mode
#ifdef FEAT_QUICKFIX
		// When splitting the quickfix window open a new buffer in it,
		// don't replicate the quickfix buffer.
		if (bt_quickfix(curbuf))
		    goto newwindow;
#endif
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		(void)win_split((int)Prenum, WSP_VERT);
		break;

// split current window and edit alternate file
    case Ctrl_HAT:
    case '^':
		CHECK_CMDWIN;
		reset_VIsual_and_resel();	// stop Visual mode

		if (buflist_findnr(Prenum == 0
					? curwin->w_alt_fnum : Prenum) == NULL)
		{
		    if (Prenum == 0)
			emsg(_(e_no_alternate_file));
		    else
			semsg(_(e_buffer_nr_not_found), Prenum);
		    break;
		}

		if (!curbuf_locked() && win_split(0, 0) == OK)
		    (void)buflist_getfile(
			    Prenum == 0 ? curwin->w_alt_fnum : Prenum,
			    (linenr_T)0, GETF_ALT, FALSE);
		break;

// open new window
    case Ctrl_N:
    case 'n':
		CHECK_CMDWIN;
		reset_VIsual_and_resel();	// stop Visual mode
#ifdef FEAT_QUICKFIX
newwindow:
#endif
		if (Prenum)
		    // window height
		    vim_snprintf((char *)cbuf, sizeof(cbuf) - 5, "%ld", Prenum);
		else
		    cbuf[0] = NUL;
#if defined(FEAT_QUICKFIX)
		if (nchar == 'v' || nchar == Ctrl_V)
		    STRCAT(cbuf, "v");
#endif
		STRCAT(cbuf, "new");
		do_cmdline_cmd(cbuf);
		break;

// quit current window
    case Ctrl_Q:
    case 'q':
		reset_VIsual_and_resel();	// stop Visual mode
		cmd_with_count("quit", cbuf, sizeof(cbuf), Prenum);
		do_cmdline_cmd(cbuf);
		break;

// close current window
    case Ctrl_C:
    case 'c':
		reset_VIsual_and_resel();	// stop Visual mode
		cmd_with_count("close", cbuf, sizeof(cbuf), Prenum);
		do_cmdline_cmd(cbuf);
		break;

#if defined(FEAT_QUICKFIX)
// close preview window
    case Ctrl_Z:
    case 'z':
		CHECK_CMDWIN;
		reset_VIsual_and_resel();	// stop Visual mode
		do_cmdline_cmd((char_u *)"pclose");
		break;

// cursor to preview window
    case 'P':
		FOR_ALL_WINDOWS(wp)
		    if (wp->w_p_pvw)
			break;
		if (wp == NULL)
		    emsg(_(e_there_is_no_preview_window));
		else
		    win_goto(wp);
		break;
#endif

// close all but current window
    case Ctrl_O:
    case 'o':
		CHECK_CMDWIN;
		reset_VIsual_and_resel();	// stop Visual mode
		cmd_with_count("only", cbuf, sizeof(cbuf), Prenum);
		do_cmdline_cmd(cbuf);
		break;

// cursor to next window with wrap around
    case Ctrl_W:
    case 'w':
// cursor to previous window with wrap around
    case 'W':
		CHECK_CMDWIN;
		if (ONE_WINDOW && Prenum != 1)	// just one window
		    beep_flush();
		else
		{
		    if (Prenum)			// go to specified window
		    {
			for (wp = firstwin; --Prenum > 0; )
			{
			    if (wp->w_next == NULL)
				break;
			    else
				wp = wp->w_next;
			}
		    }
		    else
		    {
			if (nchar == 'W')	    // go to previous window
			{
			    wp = curwin->w_prev;
			    if (wp == NULL)
				wp = lastwin;	    // wrap around
			}
			else			    // go to next window
			{
			    wp = curwin->w_next;
			    if (wp == NULL)
				wp = firstwin;	    // wrap around
			}
		    }
		    win_goto(wp);
		}
		break;

// cursor to window below
    case 'j':
    case K_DOWN:
    case Ctrl_J:
		CHECK_CMDWIN;
		win_goto_ver(FALSE, Prenum1);
		break;

// cursor to window above
    case 'k':
    case K_UP:
    case Ctrl_K:
		CHECK_CMDWIN;
		win_goto_ver(TRUE, Prenum1);
		break;

// cursor to left window
    case 'h':
    case K_LEFT:
    case Ctrl_H:
    case K_BS:
		CHECK_CMDWIN;
		win_goto_hor(TRUE, Prenum1);
		break;

// cursor to right window
    case 'l':
    case K_RIGHT:
    case Ctrl_L:
		CHECK_CMDWIN;
		win_goto_hor(FALSE, Prenum1);
		break;

// move window to new tab page
    case 'T':
		CHECK_CMDWIN;
		if (one_window())
		    msg(_(m_onlyone));
		else
		{
		    tabpage_T	*oldtab = curtab;
		    tabpage_T	*newtab;

		    // First create a new tab with the window, then go back to
		    // the old tab and close the window there.
		    wp = curwin;
		    if (win_new_tabpage((int)Prenum) == OK
						     && valid_tabpage(oldtab))
		    {
			newtab = curtab;
			goto_tabpage_tp(oldtab, TRUE, TRUE);
			if (curwin == wp)
			    win_close(curwin, FALSE);
			if (valid_tabpage(newtab))
			    goto_tabpage_tp(newtab, TRUE, TRUE);
		    }
		}
		break;

// cursor to top-left window
    case 't':
    case Ctrl_T:
		win_goto(firstwin);
		break;

// cursor to bottom-right window
    case 'b':
    case Ctrl_B:
		win_goto(lastwin);
		break;

// cursor to last accessed (previous) window
    case 'p':
    case Ctrl_P:
		if (!win_valid(prevwin))
		    beep_flush();
		else
		    win_goto(prevwin);
		break;

// exchange current and next window
    case 'x':
    case Ctrl_X:
		CHECK_CMDWIN;
		win_exchange(Prenum);
		break;

// rotate windows downwards
    case Ctrl_R:
    case 'r':
		CHECK_CMDWIN;
		reset_VIsual_and_resel();	// stop Visual mode
		win_rotate(FALSE, (int)Prenum1);    // downwards
		break;

// rotate windows upwards
    case 'R':
		CHECK_CMDWIN;
		reset_VIsual_and_resel();	// stop Visual mode
		win_rotate(TRUE, (int)Prenum1);	    // upwards
		break;

// move window to the very top/bottom/left/right
    case 'K':
    case 'J':
    case 'H':
    case 'L':
		CHECK_CMDWIN;
		win_totop((int)Prenum,
			((nchar == 'H' || nchar == 'L') ? WSP_VERT : 0)
			| ((nchar == 'H' || nchar == 'K') ? WSP_TOP : WSP_BOT));
		break;

// make all windows the same height
    case '=':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_equal(NULL, FALSE, 'b');
		break;

// increase current window height
    case '+':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setheight(curwin->w_height + (int)Prenum1);
		break;

// decrease current window height
    case '-':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setheight(curwin->w_height - (int)Prenum1);
		break;

// set current window height
    case Ctrl__:
    case '_':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setheight(Prenum ? (int)Prenum : 9999);
		break;

// increase current window width
    case '>':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setwidth(curwin->w_width + (int)Prenum1);
		break;

// decrease current window width
    case '<':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setwidth(curwin->w_width - (int)Prenum1);
		break;

// set current window width
    case '|':
#ifdef FEAT_GUI
		need_mouse_correct = TRUE;
#endif
		win_setwidth(Prenum != 0 ? (int)Prenum : 9999);
		break;

// jump to tag and split window if tag exists (in preview window)
#if defined(FEAT_QUICKFIX)
    case '}':
		CHECK_CMDWIN;
		if (Prenum)
		    g_do_tagpreview = Prenum;
		else
		    g_do_tagpreview = p_pvh;
#endif
		// FALLTHROUGH
    case ']':
    case Ctrl_RSB:
		CHECK_CMDWIN;
		// keep Visual mode, can select words to use as a tag
		if (Prenum)
		    postponed_split = Prenum;
		else
		    postponed_split = -1;
#ifdef FEAT_QUICKFIX
		if (nchar != '}')
		    g_do_tagpreview = 0;
#endif

		// Execute the command right here, required when "wincmd ]"
		// was used in a function.
		do_nv_ident(Ctrl_RSB, NUL);
		break;

#ifdef FEAT_SEARCHPATH
// edit file name under cursor in a new window
    case 'f':
    case 'F':
    case Ctrl_F:
wingotofile:
		CHECK_CMDWIN;

		ptr = grab_file_name(Prenum1, &lnum);
		if (ptr != NULL)
		{
		    tabpage_T	*oldtab = curtab;
		    win_T	*oldwin = curwin;
# ifdef FEAT_GUI
		    need_mouse_correct = TRUE;
# endif
		    setpcmark();
		    if (win_split(0, 0) == OK)
		    {
			RESET_BINDING(curwin);
			if (do_ecmd(0, ptr, NULL, NULL, ECMD_LASTL,
						   ECMD_HIDE, NULL) == FAIL)
			{
			    // Failed to open the file, close the window
			    // opened for it.
			    win_close(curwin, FALSE);
			    goto_tabpage_win(oldtab, oldwin);
			}
			else if (nchar == 'F' && lnum >= 0)
			{
			    curwin->w_cursor.lnum = lnum;
			    check_cursor_lnum();
			    beginline(BL_SOL | BL_FIX);
			}
		    }
		    vim_free(ptr);
		}
		break;
#endif

#ifdef FEAT_FIND_ID
// Go to the first occurrence of the identifier under cursor along path in a
// new window -- webb
    case 'i':			    // Go to any match
    case Ctrl_I:
		type = FIND_ANY;
		// FALLTHROUGH
    case 'd':			    // Go to definition, using 'define'
    case Ctrl_D:
		CHECK_CMDWIN;
		if ((len = find_ident_under_cursor(&ptr, FIND_IDENT)) == 0)
		    break;
		find_pattern_in_path(ptr, 0, len, TRUE,
			Prenum == 0 ? TRUE : FALSE, type,
			Prenum1, ACTION_SPLIT, (linenr_T)1, (linenr_T)MAXLNUM);
		curwin->w_set_curswant = TRUE;
		break;
#endif

// Quickfix window only: view the result under the cursor in a new split.
#if defined(FEAT_QUICKFIX)
    case K_KENTER:
    case CAR:
		if (bt_quickfix(curbuf))
		    qf_view_result(TRUE);
		break;
#endif

// CTRL-W g  extended commands
    case 'g':
    case Ctrl_G:
		CHECK_CMDWIN;
#ifdef USE_ON_FLY_SCROLL
		dont_scroll = TRUE;		// disallow scrolling here
#endif
		++no_mapping;
		++allow_keys;   // no mapping for xchar, but allow key codes
		if (xchar == NUL)
		    xchar = plain_vgetc();
		LANGMAP_ADJUST(xchar, TRUE);
		--no_mapping;
		--allow_keys;
#ifdef FEAT_CMDL_INFO
		(void)add_to_showcmd(xchar);
#endif
		switch (xchar)
		{
#if defined(FEAT_QUICKFIX)
		    case '}':
			xchar = Ctrl_RSB;
			if (Prenum)
			    g_do_tagpreview = Prenum;
			else
			    g_do_tagpreview = p_pvh;
#endif
			// FALLTHROUGH
		    case ']':
		    case Ctrl_RSB:
			// keep Visual mode, can select words to use as a tag
			if (Prenum)
			    postponed_split = Prenum;
			else
			    postponed_split = -1;

			// Execute the command right here, required when
			// "wincmd g}" was used in a function.
			do_nv_ident('g', xchar);
			break;

#ifdef FEAT_SEARCHPATH
		    case 'f':	    // CTRL-W gf: "gf" in a new tab page
		    case 'F':	    // CTRL-W gF: "gF" in a new tab page
			cmdmod.cmod_tab = tabpage_index(curtab) + 1;
			nchar = xchar;
			goto wingotofile;
#endif
		    case 't':	    // CTRL-W gt: go to next tab page
			goto_tabpage((int)Prenum);
			break;

		    case 'T':	    // CTRL-W gT: go to previous tab page
			goto_tabpage(-(int)Prenum1);
			break;

		    case TAB:	    // CTRL-W g<Tab>: go to last used tab page
			if (goto_tabpage_lastused() == FAIL)
			    beep_flush();
			break;

		    default:
			beep_flush();
			break;
		}
		break;

    default:	beep_flush();
		break;
    }
}