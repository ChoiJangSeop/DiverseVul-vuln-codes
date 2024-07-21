send_init_strings(int fd GCC_UNUSED, TTY * old_settings)
{
    int i;
    bool need_flush = FALSE;

    (void) old_settings;
#ifdef TAB3
    if (old_settings != 0 &&
	old_settings->c_oflag & (TAB3 | ONLCR | OCRNL | ONLRET)) {
	old_settings->c_oflag &= (TAB3 | ONLCR | OCRNL | ONLRET);
	SET_TTY(fd, old_settings);
    }
#endif
    if (use_reset || use_init) {
	if (VALID_STRING(init_prog)) {
	    IGNORE_RC(system(init_prog));
	}

	need_flush |= sent_string((use_reset && (reset_1string != 0))
				  ? reset_1string
				  : init_1string);

	need_flush |= sent_string((use_reset && (reset_2string != 0))
				  ? reset_2string
				  : init_2string);

	if (VALID_STRING(clear_margins)) {
	    need_flush |= sent_string(clear_margins);
	} else
#if defined(set_lr_margin)
	if (VALID_STRING(set_lr_margin)) {
	    need_flush |= sent_string(TPARM_2(set_lr_margin, 0,
					      columns - 1));
	} else
#endif
#if defined(set_left_margin_parm) && defined(set_right_margin_parm)
	    if (VALID_STRING(set_left_margin_parm)
		&& VALID_STRING(set_right_margin_parm)) {
	    need_flush |= sent_string(TPARM_1(set_left_margin_parm, 0));
	    need_flush |= sent_string(TPARM_1(set_right_margin_parm,
					      columns - 1));
	} else
#endif
	    if (VALID_STRING(set_left_margin)
		&& VALID_STRING(set_right_margin)) {
	    need_flush |= to_left_margin();
	    need_flush |= sent_string(set_left_margin);
	    if (VALID_STRING(parm_right_cursor)) {
		need_flush |= sent_string(TPARM_1(parm_right_cursor,
						  columns - 1));
	    } else {
		for (i = 0; i < columns - 1; i++) {
		    out_char(' ');
		    need_flush = TRUE;
		}
	    }
	    need_flush |= sent_string(set_right_margin);
	    need_flush |= to_left_margin();
	}

	need_flush |= reset_tabstops(columns);

	need_flush |= cat_file((use_reset && reset_file) ? reset_file : init_file);

	need_flush |= sent_string((use_reset && (reset_3string != 0))
				  ? reset_3string
				  : init_3string);
    }

    return need_flush;
}