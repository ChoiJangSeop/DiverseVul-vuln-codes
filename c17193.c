relative_move(NCURSES_SP_DCLx
	      string_desc * target,
	      int from_y,
	      int from_x,
	      int to_y,
	      int to_x,
	      int ovw)
/* move via local motions (cuu/cuu1/cud/cud1/cub1/cub/cuf1/cuf/vpa/hpa) */
{
    string_desc save;
    int n, vcost = 0, hcost = 0;

    (void) _nc_str_copy(&save, target);

    if (to_y != from_y) {
	vcost = INFINITY;

	if (row_address != 0
	    && _nc_safe_strcat(target, TPARM_1(row_address, to_y))) {
	    vcost = SP_PARM->_vpa_cost;
	}

	if (to_y > from_y) {
	    n = (to_y - from_y);

	    if (parm_down_cursor
		&& SP_PARM->_cud_cost < vcost
		&& _nc_safe_strcat(_nc_str_copy(target, &save),
				   TPARM_1(parm_down_cursor, n))) {
		vcost = SP_PARM->_cud_cost;
	    }

	    if (cursor_down
		&& (*cursor_down != '\n' || SP_PARM->_nl)
		&& (n * SP_PARM->_cud1_cost < vcost)) {
		vcost = repeated_append(_nc_str_copy(target, &save), 0,
					SP_PARM->_cud1_cost, n, cursor_down);
	    }
	} else {		/* (to_y < from_y) */
	    n = (from_y - to_y);

	    if (parm_up_cursor
		&& SP_PARM->_cuu_cost < vcost
		&& _nc_safe_strcat(_nc_str_copy(target, &save),
				   TPARM_1(parm_up_cursor, n))) {
		vcost = SP_PARM->_cuu_cost;
	    }

	    if (cursor_up && (n * SP_PARM->_cuu1_cost < vcost)) {
		vcost = repeated_append(_nc_str_copy(target, &save), 0,
					SP_PARM->_cuu1_cost, n, cursor_up);
	    }
	}

	if (vcost == INFINITY)
	    return (INFINITY);
    }

    save = *target;

    if (to_x != from_x) {
	char str[OPT_SIZE];
	string_desc check;

	hcost = INFINITY;

	if (column_address
	    && _nc_safe_strcat(_nc_str_copy(target, &save),
			       TPARM_1(column_address, to_x))) {
	    hcost = SP_PARM->_hpa_cost;
	}

	if (to_x > from_x) {
	    n = to_x - from_x;

	    if (parm_right_cursor
		&& SP_PARM->_cuf_cost < hcost
		&& _nc_safe_strcat(_nc_str_copy(target, &save),
				   TPARM_1(parm_right_cursor, n))) {
		hcost = SP_PARM->_cuf_cost;
	    }

	    if (cursor_right) {
		int lhcost = 0;

		(void) _nc_str_init(&check, str, sizeof(str));

#if USE_HARD_TABS
		/* use hard tabs, if we have them, to do as much as possible */
		if (init_tabs > 0 && tab) {
		    int nxt, fr;

		    for (fr = from_x; (nxt = NEXTTAB(fr)) <= to_x; fr = nxt) {
			lhcost = repeated_append(&check, lhcost,
						 SP_PARM->_ht_cost, 1, tab);
			if (lhcost == INFINITY)
			    break;
		    }

		    n = to_x - fr;
		    from_x = fr;
		}
#endif /* USE_HARD_TABS */

		if (n <= 0 || n >= (int) check.s_size)
		    ovw = FALSE;
#if BSD_TPUTS
		/*
		 * If we're allowing BSD-style padding in tputs, don't generate
		 * a string with a leading digit.  Otherwise, that will be
		 * interpreted as a padding value rather than sent to the
		 * screen.
		 */
		if (ovw
		    && n > 0
		    && n < (int) check.s_size
		    && vcost == 0
		    && str[0] == '\0') {
		    int wanted = CharOf(WANT_CHAR(SP_PARM, to_y, from_x));
		    if (is8bits(wanted) && isdigit(wanted))
			ovw = FALSE;
		}
#endif
		/*
		 * If we have no attribute changes, overwrite is cheaper.
		 * Note: must suppress this by passing in ovw = FALSE whenever
		 * WANT_CHAR would return invalid data.  In particular, this
		 * is true between the time a hardware scroll has been done
		 * and the time the structure WANT_CHAR would access has been
		 * updated.
		 */
		if (ovw) {
		    int i;

		    for (i = 0; i < n; i++) {
			NCURSES_CH_T ch = WANT_CHAR(SP_PARM, to_y, from_x + i);
			if (!SameAttrOf(ch, SCREEN_ATTRS(SP_PARM))
#if USE_WIDEC_SUPPORT
			    || !Charable(ch)
#endif
			    ) {
			    ovw = FALSE;
			    break;
			}
		    }
		}
		if (ovw) {
		    int i;

		    for (i = 0; i < n; i++)
			*check.s_tail++ = (char) CharOf(WANT_CHAR(SP_PARM, to_y,
								  from_x + i));
		    *check.s_tail = '\0';
		    check.s_size -= (size_t) n;
		    lhcost += n * SP_PARM->_char_padding;
		} else {
		    lhcost = repeated_append(&check, lhcost, SP_PARM->_cuf1_cost,
					     n, cursor_right);
		}

		if (lhcost < hcost
		    && _nc_safe_strcat(_nc_str_copy(target, &save), str)) {
		    hcost = lhcost;
		}
	    }
	} else {		/* (to_x < from_x) */
	    n = from_x - to_x;

	    if (parm_left_cursor
		&& SP_PARM->_cub_cost < hcost
		&& _nc_safe_strcat(_nc_str_copy(target, &save),
				   TPARM_1(parm_left_cursor, n))) {
		hcost = SP_PARM->_cub_cost;
	    }

	    if (cursor_left) {
		int lhcost = 0;

		(void) _nc_str_init(&check, str, sizeof(str));

#if USE_HARD_TABS
		if (init_tabs > 0 && back_tab) {
		    int nxt, fr;

		    for (fr = from_x; (nxt = LASTTAB(fr)) >= to_x; fr = nxt) {
			lhcost = repeated_append(&check, lhcost,
						 SP_PARM->_cbt_cost,
						 1, back_tab);
			if (lhcost == INFINITY)
			    break;
		    }

		    n = fr - to_x;
		}
#endif /* USE_HARD_TABS */

		lhcost = repeated_append(&check, lhcost,
					 SP_PARM->_cub1_cost,
					 n, cursor_left);

		if (lhcost < hcost
		    && _nc_safe_strcat(_nc_str_copy(target, &save), str)) {
		    hcost = lhcost;
		}
	    }
	}

	if (hcost == INFINITY)
	    return (INFINITY);
    }

    return (vcost + hcost);
}