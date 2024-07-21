input_csi_dispatch(struct input_ctx *ictx)
{
	struct screen_write_ctx	       *sctx = &ictx->ctx;
	struct screen		       *s = sctx->s;
	struct input_table_entry       *entry;
	int				n, m;

	if (ictx->flags & INPUT_DISCARD)
		return (0);
	if (input_split(ictx) != 0)
		return (0);
	log_debug("%s: '%c' \"%s\" \"%s\"",
	    __func__, ictx->ch, ictx->interm_buf, ictx->param_buf);

	entry = bsearch(ictx, input_csi_table, nitems(input_csi_table),
	    sizeof input_csi_table[0], input_table_compare);
	if (entry == NULL) {
		log_debug("%s: unknown '%c'", __func__, ictx->ch);
		return (0);
	}

	switch (entry->type) {
	case INPUT_CSI_CBT:
		/* Find the previous tab point, n times. */
		n = input_get(ictx, 0, 1, 1);
		while (s->cx > 0 && n-- > 0) {
			do
				s->cx--;
			while (s->cx > 0 && !bit_test(s->tabs, s->cx));
		}
		break;
	case INPUT_CSI_CUB:
		screen_write_cursorleft(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_CUD:
		screen_write_cursordown(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_CUF:
		screen_write_cursorright(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_CUP:
		n = input_get(ictx, 0, 1, 1);
		m = input_get(ictx, 1, 1, 1);
		screen_write_cursormove(sctx, m - 1, n - 1);
		break;
	case INPUT_CSI_WINOPS:
		input_csi_dispatch_winops(ictx);
		break;
	case INPUT_CSI_CUU:
		screen_write_cursorup(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_CNL:
		screen_write_carriagereturn(sctx);
		screen_write_cursordown(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_CPL:
		screen_write_carriagereturn(sctx);
		screen_write_cursorup(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_DA:
		switch (input_get(ictx, 0, 0, 0)) {
		case 0:
			input_reply(ictx, "\033[?1;2c");
			break;
		default:
			log_debug("%s: unknown '%c'", __func__, ictx->ch);
			break;
		}
		break;
	case INPUT_CSI_DA_TWO:
		switch (input_get(ictx, 0, 0, 0)) {
		case 0:
			input_reply(ictx, "\033[>84;0;0c");
			break;
		default:
			log_debug("%s: unknown '%c'", __func__, ictx->ch);
			break;
		}
		break;
	case INPUT_CSI_ECH:
		screen_write_clearcharacter(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_DCH:
		screen_write_deletecharacter(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_DECSTBM:
		n = input_get(ictx, 0, 1, 1);
		m = input_get(ictx, 1, 1, screen_size_y(s));
		screen_write_scrollregion(sctx, n - 1, m - 1);
		break;
	case INPUT_CSI_DL:
		screen_write_deleteline(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_DSR:
		switch (input_get(ictx, 0, 0, 0)) {
		case 5:
			input_reply(ictx, "\033[0n");
			break;
		case 6:
			input_reply(ictx, "\033[%u;%uR", s->cy + 1, s->cx + 1);
			break;
		default:
			log_debug("%s: unknown '%c'", __func__, ictx->ch);
			break;
		}
		break;
	case INPUT_CSI_ED:
		switch (input_get(ictx, 0, 0, 0)) {
		case 0:
			screen_write_clearendofscreen(sctx);
			break;
		case 1:
			screen_write_clearstartofscreen(sctx);
			break;
		case 2:
			screen_write_clearscreen(sctx);
			break;
		case 3:
			switch (input_get(ictx, 1, 0, 0)) {
			case 0:
				/*
				 * Linux console extension to clear history
				 * (for example before locking the screen).
				 */
				screen_write_clearhistory(sctx);
				break;
			}
			break;
		default:
			log_debug("%s: unknown '%c'", __func__, ictx->ch);
			break;
		}
		break;
	case INPUT_CSI_EL:
		switch (input_get(ictx, 0, 0, 0)) {
		case 0:
			screen_write_clearendofline(sctx);
			break;
		case 1:
			screen_write_clearstartofline(sctx);
			break;
		case 2:
			screen_write_clearline(sctx);
			break;
		default:
			log_debug("%s: unknown '%c'", __func__, ictx->ch);
			break;
		}
		break;
	case INPUT_CSI_HPA:
		n = input_get(ictx, 0, 1, 1);
		screen_write_cursormove(sctx, n - 1, s->cy);
		break;
	case INPUT_CSI_ICH:
		screen_write_insertcharacter(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_IL:
		screen_write_insertline(sctx, input_get(ictx, 0, 1, 1));
		break;
	case INPUT_CSI_RCP:
		memcpy(&ictx->cell, &ictx->old_cell, sizeof ictx->cell);
		screen_write_cursormove(sctx, ictx->old_cx, ictx->old_cy);
		break;
	case INPUT_CSI_RM:
		input_csi_dispatch_rm(ictx);
		break;
	case INPUT_CSI_RM_PRIVATE:
		input_csi_dispatch_rm_private(ictx);
		break;
	case INPUT_CSI_SCP:
		memcpy(&ictx->old_cell, &ictx->cell, sizeof ictx->old_cell);
		ictx->old_cx = s->cx;
		ictx->old_cy = s->cy;
		break;
	case INPUT_CSI_SGR:
		input_csi_dispatch_sgr(ictx);
		break;
	case INPUT_CSI_SM:
		input_csi_dispatch_sm(ictx);
		break;
	case INPUT_CSI_SM_PRIVATE:
		input_csi_dispatch_sm_private(ictx);
		break;
	case INPUT_CSI_TBC:
		switch (input_get(ictx, 0, 0, 0)) {
		case 0:
			if (s->cx < screen_size_x(s))
				bit_clear(s->tabs, s->cx);
			break;
		case 3:
			bit_nclear(s->tabs, 0, screen_size_x(s) - 1);
			break;
		default:
			log_debug("%s: unknown '%c'", __func__, ictx->ch);
			break;
		}
		break;
	case INPUT_CSI_VPA:
		n = input_get(ictx, 0, 1, 1);
		screen_write_cursormove(sctx, s->cx, n - 1);
		break;
	case INPUT_CSI_DECSCUSR:
		n = input_get(ictx, 0, 0, 0);
		screen_set_cursor_style(s, n);
		break;
	}

	return (0);
}