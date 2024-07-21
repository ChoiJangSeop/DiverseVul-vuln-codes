NCURSES_SP_NAME(_nc_mvcur_init) (NCURSES_SP_DCL0)
/* initialize the cost structure */
{
    if (SP_PARM->_ofp && NC_ISATTY(fileno(SP_PARM->_ofp))) {
	SP_PARM->_char_padding = ((BAUDBYTE * 1000 * 10)
				  / (BAUDRATE(SP_PARM) > 0
				     ? BAUDRATE(SP_PARM)
				     : 9600));
    } else {
	SP_PARM->_char_padding = 1;	/* must be nonzero */
    }
    if (SP_PARM->_char_padding <= 0)
	SP_PARM->_char_padding = 1;	/* must be nonzero */
    TR(TRACE_CHARPUT | TRACE_MOVE, ("char_padding %d msecs", SP_PARM->_char_padding));

    /* non-parameterized local-motion strings */
    SP_PARM->_cr_cost = CostOf(carriage_return, 0);
    SP_PARM->_home_cost = CostOf(cursor_home, 0);
    SP_PARM->_ll_cost = CostOf(cursor_to_ll, 0);
#if USE_HARD_TABS
    if (getenv("NCURSES_NO_HARD_TABS") == 0
	&& dest_tabs_magic_smso == 0
	&& HasHardTabs()) {
	SP_PARM->_ht_cost = CostOf(tab, 0);
	SP_PARM->_cbt_cost = CostOf(back_tab, 0);
    } else {
	SP_PARM->_ht_cost = INFINITY;
	SP_PARM->_cbt_cost = INFINITY;
    }
#endif /* USE_HARD_TABS */
    SP_PARM->_cub1_cost = CostOf(cursor_left, 0);
    SP_PARM->_cuf1_cost = CostOf(cursor_right, 0);
    SP_PARM->_cud1_cost = CostOf(cursor_down, 0);
    SP_PARM->_cuu1_cost = CostOf(cursor_up, 0);

    SP_PARM->_smir_cost = CostOf(enter_insert_mode, 0);
    SP_PARM->_rmir_cost = CostOf(exit_insert_mode, 0);
    SP_PARM->_ip_cost = 0;
    if (insert_padding) {
	SP_PARM->_ip_cost = CostOf(insert_padding, 0);
    }

    /*
     * Assumption: if the terminal has memory_relative addressing, the
     * initialization strings or smcup will set single-page mode so we
     * can treat it like absolute screen addressing.  This seems to be true
     * for all cursor_mem_address terminal types in the terminfo database.
     */
    SP_PARM->_address_cursor = cursor_address ? cursor_address : cursor_mem_address;

    /*
     * Parametrized local-motion strings.  This static cost computation
     * depends on the following assumptions:
     *
     * (1) They never have * padding.  In the entire master terminfo database
     *     as of March 1995, only the obsolete Zenith Z-100 pc violates this.
     *     (Proportional padding is found mainly in insert, delete and scroll
     *     capabilities).
     *
     * (2) The average case of cup has two two-digit parameters.  Strictly,
     *     the average case for a 24 * 80 screen has ((10*10*(1 + 1)) +
     *     (14*10*(1 + 2)) + (10*70*(2 + 1)) + (14*70*4)) / (24*80) = 3.458
     *     digits of parameters.  On a 25x80 screen the average is 3.6197.
     *     On larger screens the value gets much closer to 4.
     *
     * (3) The average case of cub/cuf/hpa/ech/rep has 2 digits of parameters
     *     (strictly, (((10 * 1) + (70 * 2)) / 80) = 1.8750).
     *
     * (4) The average case of cud/cuu/vpa has 2 digits of parameters
     *     (strictly, (((10 * 1) + (14 * 2)) / 24) = 1.5833).
     *
     * All these averages depend on the assumption that all parameter values
     * are equally probable.
     */
    SP_PARM->_cup_cost = CostOf(TPARM_2(SP_PARM->_address_cursor, 23, 23), 1);
    SP_PARM->_cub_cost = CostOf(TPARM_1(parm_left_cursor, 23), 1);
    SP_PARM->_cuf_cost = CostOf(TPARM_1(parm_right_cursor, 23), 1);
    SP_PARM->_cud_cost = CostOf(TPARM_1(parm_down_cursor, 23), 1);
    SP_PARM->_cuu_cost = CostOf(TPARM_1(parm_up_cursor, 23), 1);
    SP_PARM->_hpa_cost = CostOf(TPARM_1(column_address, 23), 1);
    SP_PARM->_vpa_cost = CostOf(TPARM_1(row_address, 23), 1);

    /* non-parameterized screen-update strings */
    SP_PARM->_ed_cost = NormalizedCost(clr_eos, 1);
    SP_PARM->_el_cost = NormalizedCost(clr_eol, 1);
    SP_PARM->_el1_cost = NormalizedCost(clr_bol, 1);
    SP_PARM->_dch1_cost = NormalizedCost(delete_character, 1);
    SP_PARM->_ich1_cost = NormalizedCost(insert_character, 1);

    /*
     * If this is a bce-terminal, we want to bias the choice so we use clr_eol
     * rather than spaces at the end of a line.
     */
    if (back_color_erase)
	SP_PARM->_el_cost = 0;

    /* parameterized screen-update strings */
    SP_PARM->_dch_cost = NormalizedCost(TPARM_1(parm_dch, 23), 1);
    SP_PARM->_ich_cost = NormalizedCost(TPARM_1(parm_ich, 23), 1);
    SP_PARM->_ech_cost = NormalizedCost(TPARM_1(erase_chars, 23), 1);
    SP_PARM->_rep_cost = NormalizedCost(TPARM_2(repeat_char, ' ', 23), 1);

    SP_PARM->_cup_ch_cost = NormalizedCost(
					      TPARM_2(SP_PARM->_address_cursor,
						      23, 23),
					      1);
    SP_PARM->_hpa_ch_cost = NormalizedCost(TPARM_1(column_address, 23), 1);
    SP_PARM->_cuf_ch_cost = NormalizedCost(TPARM_1(parm_right_cursor, 23), 1);
    SP_PARM->_inline_cost = min(SP_PARM->_cup_ch_cost,
				min(SP_PARM->_hpa_ch_cost,
				    SP_PARM->_cuf_ch_cost));

    /*
     * If save_cursor is used within enter_ca_mode, we should not use it for
     * scrolling optimization, since the corresponding restore_cursor is not
     * nested on the various terminals (vt100, xterm, etc.) which use this
     * feature.
     */
    if (save_cursor != 0
	&& enter_ca_mode != 0
	&& strstr(enter_ca_mode, save_cursor) != 0) {
	T(("...suppressed sc/rc capability due to conflict with smcup/rmcup"));
	save_cursor = 0;
	restore_cursor = 0;
    }

    /*
     * A different, possibly better way to arrange this would be to set the
     * SCREEN's _endwin at window initialization time and let this be called by
     * doupdate's return-from-shellout code.
     */
    NCURSES_SP_NAME(_nc_mvcur_resume) (NCURSES_SP_ARG);
}