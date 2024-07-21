check_termtype(TERMTYPE2 *tp, bool literal)
{
    unsigned j;

    check_conflict(tp);

    for_each_string(j, tp) {
	char *a = tp->Strings[j];
	if (VALID_STRING(a)) {
	    const char *name = ExtStrname(tp, (int) j, strnames);
	    /*
	     * If we expect parameters, or if there might be parameters,
	     * check for consistent number of parameters.
	     */
	    if (j >= SIZEOF(parametrized) ||
		is_user_capability(name) >= 0 ||
		parametrized[j] > 0) {
		check_params(tp, name, a, (j >= STRCOUNT));
	    }
	    check_delays(tp, ExtStrname(tp, (int) j, strnames), a);
	    if (capdump) {
		check_infotocap(tp, (int) j, a);
	    }
	}
    }
#if NCURSES_XNAMES
    /* in extended mode, verify that each extension is expected type */
    for_each_ext_boolean(j, tp) {
	check_user_capability_type(ExtBoolname(tp, (int) j, strnames), BOOLEAN);
    }
    for_each_ext_number(j, tp) {
	check_user_capability_type(ExtNumname(tp, (int) j, strnames), NUMBER);
    }
    for_each_ext_string(j, tp) {
	check_user_capability_type(ExtStrname(tp, (int) j, strnames), STRING);
    }
#endif /* NCURSES_XNAMES */

    check_acs(tp);
    check_colors(tp);
    check_cursor(tp);
    check_keypad(tp);
    check_printer(tp);
    check_screen(tp);

    /*
     * These are probably both or none.
     */
    PAIRED(parm_index, parm_rindex);
    PAIRED(parm_ich, parm_dch);

    /*
     * These may be mismatched because the terminal description relies on
     * restoring the cursor visibility by resetting it.
     */
    ANDMISSING(cursor_invisible, cursor_normal);
    ANDMISSING(cursor_visible, cursor_normal);

    if (PRESENT(cursor_visible) && PRESENT(cursor_normal)
	&& !_nc_capcmp(cursor_visible, cursor_normal))
	_nc_warning("cursor_visible is same as cursor_normal");

    /*
     * From XSI & O'Reilly, we gather that sc/rc are required if csr is
     * given, because the cursor position after the scrolling operation is
     * performed is undefined.
     */
    ANDMISSING(change_scroll_region, save_cursor);
    ANDMISSING(change_scroll_region, restore_cursor);

    /*
     * If we can clear tabs, we should be able to initialize them.
     */
    ANDMISSING(clear_all_tabs, set_tab);

    if (PRESENT(set_attributes)) {
	char *zero = 0;

	_nc_tparm_err = 0;
	if (PRESENT(exit_attribute_mode)) {
	    zero = strdup(CHECK_SGR(0, exit_attribute_mode));
	} else {
	    zero = strdup(TPARM_9(set_attributes, 0, 0, 0, 0, 0, 0, 0, 0, 0));
	}
	if (_nc_tparm_err)
	    _nc_warning("stack error in sgr(0) string");

	if (zero != 0) {
	    CHECK_SGR(1, enter_standout_mode);
	    CHECK_SGR(2, enter_underline_mode);
	    CHECK_SGR(3, enter_reverse_mode);
	    CHECK_SGR(4, enter_blink_mode);
	    CHECK_SGR(5, enter_dim_mode);
	    CHECK_SGR(6, enter_bold_mode);
	    CHECK_SGR(7, enter_secure_mode);
	    CHECK_SGR(8, enter_protected_mode);
	    CHECK_SGR(9, enter_alt_charset_mode);
	    free(zero);
	} else {
	    _nc_warning("sgr(0) did not return a value");
	}
    } else if (PRESENT(exit_attribute_mode) &&
	       set_attributes != CANCELLED_STRING) {
	if (_nc_syntax == SYN_TERMINFO)
	    _nc_warning("missing sgr string");
    }
#define CHECK_SGR0(name) check_exit_attribute(#name, name, check_sgr0, exit_attribute_mode)
    if (PRESENT(exit_attribute_mode)) {
	char *check_sgr0 = _nc_trim_sgr0(tp);

	if (check_sgr0 == 0 || *check_sgr0 == '\0') {
	    _nc_warning("trimmed sgr0 is empty");
	} else {
	    show_where(2);
	    if (check_sgr0 != exit_attribute_mode) {
		DEBUG(2,
		      ("will trim sgr0\n\toriginal sgr0=%s\n\ttrimmed  sgr0=%s",
		       _nc_visbuf2(1, exit_attribute_mode),
		       _nc_visbuf2(2, check_sgr0)));
	    } else {
		DEBUG(2,
		      ("will not trim sgr0\n\toriginal sgr0=%s",
		       _nc_visbuf(exit_attribute_mode)));
	    }
	}
#if defined(exit_italics_mode)
	CHECK_SGR0(exit_italics_mode);
#endif
	CHECK_SGR0(exit_standout_mode);
	CHECK_SGR0(exit_underline_mode);
	if (check_sgr0 != exit_attribute_mode) {
	    free(check_sgr0);
	}
    }
#define CHECK_SGR_PARAM(code, name) check_sgr_param(tp, (int)code, #name, name)
    for (j = 0; *sgr_names[j] != '\0'; ++j) {
	CHECK_SGR_PARAM(j, set_a_foreground);
	CHECK_SGR_PARAM(j, set_a_background);
	CHECK_SGR_PARAM(j, set_foreground);
	CHECK_SGR_PARAM(j, set_background);
    }
#ifdef TRACE
    show_where(2);
    if (!auto_right_margin) {
	DEBUG(2,
	      ("can write to lower-right directly"));
    } else if (PRESENT(enter_am_mode) && PRESENT(exit_am_mode)) {
	DEBUG(2,
	      ("can write to lower-right by suppressing automargin"));
    } else if ((PRESENT(enter_insert_mode) && PRESENT(exit_insert_mode))
	       || PRESENT(insert_character) || PRESENT(parm_ich)) {
	DEBUG(2,
	      ("can write to lower-right by using inserts"));
    } else {
	DEBUG(2,
	      ("cannot write to lower-right"));
    }
#endif

    /*
     * Some standard applications (e.g., vi) and some non-curses
     * applications (e.g., jove) get confused if we have both ich1 and
     * smir/rmir.  Let's be nice and warn about that, too, even though
     * ncurses handles it.
     */
    if ((PRESENT(enter_insert_mode) || PRESENT(exit_insert_mode))
	&& PRESENT(insert_character)) {
	_nc_warning("non-curses applications may be confused by ich1 with smir/rmir");
    }

    /*
     * Finally, do the non-verbose checks
     */
    if (save_check_termtype != 0)
	save_check_termtype(tp, literal);
}