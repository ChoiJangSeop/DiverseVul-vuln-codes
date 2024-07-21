feed_table_tag(struct table *tbl, char *line, struct table_mode *mode,
	       int width, struct parsed_tag *tag)
{
    int cmd;
#ifdef ID_EXT
    char *p;
#endif
    struct table_cell *cell = &tbl->cell;
    int colspan, rowspan;
    int col, prev_col;
    int i, j, k, v, v0, w, id;
    Str tok, tmp, anchor;
    table_attr align, valign;

    cmd = tag->tagid;

    if (mode->pre_mode & TBLM_PLAIN) {
	if (mode->end_tag == cmd) {
	    mode->pre_mode &= ~TBLM_PLAIN;
	    mode->end_tag = 0;
	    feed_table_block_tag(tbl, line, mode, 0, cmd);
	    return TAG_ACTION_NONE;
	}
	return TAG_ACTION_PLAIN;
    }
    if (mode->pre_mode & TBLM_INTXTA) {
	switch (cmd) {
	CASE_TABLE_TAG:
	case HTML_N_TEXTAREA:
	    table_close_textarea(tbl, mode, width);
	    if (cmd == HTML_N_TEXTAREA)
		return TAG_ACTION_NONE;
	    break;
	default:
	    return TAG_ACTION_FEED;
	}
    }
    if (mode->pre_mode & TBLM_SCRIPT) {
	if (mode->end_tag == cmd) {
	    mode->pre_mode &= ~TBLM_SCRIPT;
	    mode->end_tag = 0;
	    return TAG_ACTION_NONE;
	}
	return TAG_ACTION_PLAIN;
    }
    if (mode->pre_mode & TBLM_STYLE) {
	if (mode->end_tag == cmd) {
	    mode->pre_mode &= ~TBLM_STYLE;
	    mode->end_tag = 0;
	    return TAG_ACTION_NONE;
	}
	return TAG_ACTION_PLAIN;
    }
    /* failsafe: a tag other than <option></option>and </select> in *
     * <select> environment is regarded as the end of <select>. */
    if (mode->pre_mode & TBLM_INSELECT) {
	switch (cmd) {
	  CASE_TABLE_TAG:
	case HTML_N_FORM:
	case HTML_N_SELECT:	/* mode->end_tag */
	    table_close_select(tbl, mode, width);
	    if (cmd == HTML_N_SELECT)
		return TAG_ACTION_NONE;
	    break;
	default:
	    return TAG_ACTION_FEED;
	}
    }
    if (mode->caption) {
	switch (cmd) {
	  CASE_TABLE_TAG:
	case HTML_N_CAPTION:
	    mode->caption = 0;
	    if (cmd == HTML_N_CAPTION)
		return TAG_ACTION_NONE;
	    break;
	default:
	    return TAG_ACTION_FEED;
	}
    }

    if (mode->pre_mode & TBLM_PRE) {
	switch (cmd) {
	case HTML_NOBR:
	case HTML_N_NOBR:
	case HTML_PRE_INT:
	case HTML_N_PRE_INT:
	    return TAG_ACTION_NONE;
	}
    }

    switch (cmd) {
    case HTML_TABLE:
	check_rowcol(tbl, mode);
	return TAG_ACTION_TABLE;
    case HTML_N_TABLE:
	if (tbl->suspended_data)
	    check_rowcol(tbl, mode);
	return TAG_ACTION_N_TABLE;
    case HTML_TR:
	if (tbl->col >= 0 && tbl->tabcontentssize > 0)
	    setwidth(tbl, mode);
	tbl->col = -1;
	tbl->row++;
	tbl->flag |= TBL_IN_ROW;
	tbl->flag &= ~TBL_IN_COL;
	align = 0;
	valign = 0;
	if (parsedtag_get_value(tag, ATTR_ALIGN, &i)) {
	    switch (i) {
	    case ALIGN_LEFT:
		align = (HTT_LEFT | HTT_TRSET);
		break;
	    case ALIGN_RIGHT:
		align = (HTT_RIGHT | HTT_TRSET);
		break;
	    case ALIGN_CENTER:
		align = (HTT_CENTER | HTT_TRSET);
		break;
	    }
	}
	if (parsedtag_get_value(tag, ATTR_VALIGN, &i)) {
	    switch (i) {
	    case VALIGN_TOP:
		valign = (HTT_TOP | HTT_VTRSET);
		break;
	    case VALIGN_MIDDLE:
		valign = (HTT_MIDDLE | HTT_VTRSET);
		break;
	    case VALIGN_BOTTOM:
		valign = (HTT_BOTTOM | HTT_VTRSET);
		break;
	    }
	}
#ifdef ID_EXT
	if (parsedtag_get_value(tag, ATTR_ID, &p))
	    tbl->tridvalue[tbl->row] = Strnew_charp(p);
#endif				/* ID_EXT */
	tbl->trattr = align | valign;
	break;
    case HTML_TH:
    case HTML_TD:
	prev_col = tbl->col;
	if (tbl->col >= 0 && tbl->tabcontentssize > 0)
	    setwidth(tbl, mode);
	if (tbl->row == -1) {
	    /* for broken HTML... */
	    tbl->row = -1;
	    tbl->col = -1;
	    tbl->maxrow = tbl->row;
	}
	if (tbl->col == -1) {
	    if (!(tbl->flag & TBL_IN_ROW)) {
		tbl->row++;
		tbl->flag |= TBL_IN_ROW;
	    }
	    if (tbl->row > tbl->maxrow)
		tbl->maxrow = tbl->row;
	}
	tbl->col++;
	check_row(tbl, tbl->row);
	while (tbl->tabattr[tbl->row][tbl->col]) {
	    tbl->col++;
	}
	if (tbl->col > MAXCOL - 1) {
	    tbl->col = prev_col;
	    return TAG_ACTION_NONE;
	}
	if (tbl->col > tbl->maxcol) {
	    tbl->maxcol = tbl->col;
	}
	colspan = rowspan = 1;
	if (tbl->trattr & HTT_TRSET)
	    align = (tbl->trattr & HTT_ALIGN);
	else if (cmd == HTML_TH)
	    align = HTT_CENTER;
	else
	    align = HTT_LEFT;
	if (tbl->trattr & HTT_VTRSET)
	    valign = (tbl->trattr & HTT_VALIGN);
	else
	    valign = HTT_MIDDLE;
	if (parsedtag_get_value(tag, ATTR_ROWSPAN, &rowspan)) {
	    if(rowspan > ATTR_ROWSPAN_MAX) {
		rowspan = ATTR_ROWSPAN_MAX;
	    }
	    if ((tbl->row + rowspan) >= tbl->max_rowsize)
		check_row(tbl, tbl->row + rowspan);
	}
	if (parsedtag_get_value(tag, ATTR_COLSPAN, &colspan)) {
	    if ((tbl->col + colspan) >= MAXCOL) {
		/* Can't expand column */
		colspan = MAXCOL - tbl->col;
	    }
	}
	if (parsedtag_get_value(tag, ATTR_ALIGN, &i)) {
	    switch (i) {
	    case ALIGN_LEFT:
		align = HTT_LEFT;
		break;
	    case ALIGN_RIGHT:
		align = HTT_RIGHT;
		break;
	    case ALIGN_CENTER:
		align = HTT_CENTER;
		break;
	    }
	}
	if (parsedtag_get_value(tag, ATTR_VALIGN, &i)) {
	    switch (i) {
	    case VALIGN_TOP:
		valign = HTT_TOP;
		break;
	    case VALIGN_MIDDLE:
		valign = HTT_MIDDLE;
		break;
	    case VALIGN_BOTTOM:
		valign = HTT_BOTTOM;
		break;
	    }
	}
#ifdef NOWRAP
	if (parsedtag_exists(tag, ATTR_NOWRAP))
	    tbl->tabattr[tbl->row][tbl->col] |= HTT_NOWRAP;
#endif				/* NOWRAP */
	v = 0;
	if (parsedtag_get_value(tag, ATTR_WIDTH, &v)) {
#ifdef TABLE_EXPAND
	    if (v > 0) {
		if (tbl->real_width > 0)
		    v = -(v * 100) / (tbl->real_width * pixel_per_char);
		else
		    v = (int)(v / pixel_per_char);
	    }
#else
	    v = RELATIVE_WIDTH(v);
#endif				/* not TABLE_EXPAND */
	}
#ifdef ID_EXT
	if (parsedtag_get_value(tag, ATTR_ID, &p))
	    tbl->tabidvalue[tbl->row][tbl->col] = Strnew_charp(p);
#endif				/* ID_EXT */
#ifdef NOWRAP
	if (v != 0) {
	    /* NOWRAP and WIDTH= conflicts each other */
	    tbl->tabattr[tbl->row][tbl->col] &= ~HTT_NOWRAP;
	}
#endif				/* NOWRAP */
	tbl->tabattr[tbl->row][tbl->col] &= ~(HTT_ALIGN | HTT_VALIGN);
	tbl->tabattr[tbl->row][tbl->col] |= (align | valign);
	if (colspan > 1) {
	    col = tbl->col;

	    cell->icell = cell->maxcell + 1;
	    k = bsearch_2short(colspan, cell->colspan, col, cell->col, MAXCOL,
			       cell->index, cell->icell);
	    if (k <= cell->maxcell) {
		i = cell->index[k];
		if (cell->col[i] == col && cell->colspan[i] == colspan)
		    cell->icell = i;
	    }
	    if (cell->icell > cell->maxcell && cell->icell < MAXCELL) {
		cell->maxcell++;
		cell->col[cell->maxcell] = col;
		cell->colspan[cell->maxcell] = colspan;
		cell->width[cell->maxcell] = 0;
		cell->minimum_width[cell->maxcell] = 0;
		cell->fixed_width[cell->maxcell] = 0;
		if (cell->maxcell > k) {
		    int ii;
		    for (ii = cell->maxcell; ii > k; ii--)
			cell->index[ii] = cell->index[ii - 1];
		}
		cell->index[k] = cell->maxcell;
	    }
	    if (cell->icell > cell->maxcell)
		cell->icell = -1;
	}
	if (v != 0) {
	    if (colspan == 1) {
		v0 = tbl->fixed_width[tbl->col];
		if (v0 == 0 || (v0 > 0 && v > v0) || (v0 < 0 && v < v0)) {
#ifdef FEED_TABLE_DEBUG
		    fprintf(stderr, "width(%d) = %d\n", tbl->col, v);
#endif				/* TABLE_DEBUG */
		    tbl->fixed_width[tbl->col] = v;
		}
	    }
	    else if (cell->icell >= 0) {
		v0 = cell->fixed_width[cell->icell];
		if (v0 == 0 || (v0 > 0 && v > v0) || (v0 < 0 && v < v0))
		    cell->fixed_width[cell->icell] = v;
	    }
	}
	for (i = 0; i < rowspan; i++) {
	    check_row(tbl, tbl->row + i);
	    for (j = 0; j < colspan; j++) {
#if 0
		tbl->tabattr[tbl->row + i][tbl->col + j] &= ~(HTT_X | HTT_Y);
#endif
		if (!(tbl->tabattr[tbl->row + i][tbl->col + j] &
		      (HTT_X | HTT_Y))) {
		    tbl->tabattr[tbl->row + i][tbl->col + j] |=
			((i > 0) ? HTT_Y : 0) | ((j > 0) ? HTT_X : 0);
		}
		if (tbl->col + j > tbl->maxcol) {
		    tbl->maxcol = tbl->col + j;
		}
	    }
	    if (tbl->row + i > tbl->maxrow) {
		tbl->maxrow = tbl->row + i;
	    }
	}
	begin_cell(tbl, mode);
	break;
    case HTML_N_TR:
	setwidth(tbl, mode);
	tbl->col = -1;
	tbl->flag &= ~(TBL_IN_ROW | TBL_IN_COL);
	return TAG_ACTION_NONE;
    case HTML_N_TH:
    case HTML_N_TD:
	setwidth(tbl, mode);
	tbl->flag &= ~TBL_IN_COL;
#ifdef FEED_TABLE_DEBUG
	{
	    TextListItem *it;
	    int i = tbl->col, j = tbl->row;
	    fprintf(stderr, "(a) row,col: %d, %d\n", j, i);
	    if (tbl->tabdata[j] && tbl->tabdata[j][i]) {
		for (it = ((TextList *)tbl->tabdata[j][i])->first;
		     it; it = it->next)
		    fprintf(stderr, "  [%s] \n", it->ptr);
	    }
	}
#endif
	return TAG_ACTION_NONE;
    case HTML_P:
    case HTML_BR:
    case HTML_CENTER:
    case HTML_N_CENTER:
    case HTML_DIV:
    case HTML_N_DIV:
	if (!(tbl->flag & TBL_IN_ROW))
	    break;
    case HTML_DT:
    case HTML_DD:
    case HTML_H:
    case HTML_N_H:
    case HTML_LI:
    case HTML_PRE:
    case HTML_N_PRE:
    case HTML_HR:
    case HTML_LISTING:
    case HTML_XMP:
    case HTML_PLAINTEXT:
    case HTML_PRE_PLAIN:
    case HTML_N_PRE_PLAIN:
	feed_table_block_tag(tbl, line, mode, 0, cmd);
	switch (cmd) {
	case HTML_PRE:
	case HTML_PRE_PLAIN:
	    mode->pre_mode |= TBLM_PRE;
	    break;
	case HTML_N_PRE:
	case HTML_N_PRE_PLAIN:
	    mode->pre_mode &= ~TBLM_PRE;
	    break;
	case HTML_LISTING:
	    mode->pre_mode |= TBLM_PLAIN;
	    mode->end_tag = HTML_N_LISTING;
	    break;
	case HTML_XMP:
	    mode->pre_mode |= TBLM_PLAIN;
	    mode->end_tag = HTML_N_XMP;
	    break;
	case HTML_PLAINTEXT:
	    mode->pre_mode |= TBLM_PLAIN;
	    mode->end_tag = MAX_HTMLTAG;
	    break;
	}
	break;
    case HTML_DL:
    case HTML_BLQ:
    case HTML_OL:
    case HTML_UL:
	feed_table_block_tag(tbl, line, mode, 1, cmd);
	break;
    case HTML_N_DL:
    case HTML_N_BLQ:
    case HTML_N_OL:
    case HTML_N_UL:
	feed_table_block_tag(tbl, line, mode, -1, cmd);
	break;
    case HTML_NOBR:
    case HTML_WBR:
	if (!(tbl->flag & TBL_IN_ROW))
	    break;
    case HTML_PRE_INT:
	feed_table_inline_tag(tbl, line, mode, -1);
	switch (cmd) {
	case HTML_NOBR:
	    mode->nobr_level++;
	    if (mode->pre_mode & TBLM_NOBR)
		return TAG_ACTION_NONE;
	    mode->pre_mode |= TBLM_NOBR;
	    break;
	case HTML_PRE_INT:
	    if (mode->pre_mode & TBLM_PRE_INT)
		return TAG_ACTION_NONE;
	    mode->pre_mode |= TBLM_PRE_INT;
	    tbl->linfo.prev_spaces = 0;
	    break;
	}
	mode->nobr_offset = -1;
	if (tbl->linfo.length > 0) {
	    check_minimum0(tbl, tbl->linfo.length);
	    tbl->linfo.length = 0;
	}
	break;
    case HTML_N_NOBR:
	if (!(tbl->flag & TBL_IN_ROW))
	    break;
	feed_table_inline_tag(tbl, line, mode, -1);
	if (mode->nobr_level > 0)
	    mode->nobr_level--;
	if (mode->nobr_level == 0)
	    mode->pre_mode &= ~TBLM_NOBR;
	break;
    case HTML_N_PRE_INT:
	feed_table_inline_tag(tbl, line, mode, -1);
	mode->pre_mode &= ~TBLM_PRE_INT;
	break;
    case HTML_IMG:
	check_rowcol(tbl, mode);
	w = tbl->fixed_width[tbl->col];
	if (w < 0) {
	    if (tbl->total_width > 0)
		w = -tbl->total_width * w / 100;
	    else if (width > 0)
		w = -width * w / 100;
	    else
		w = 0;
	}
	else if (w == 0) {
	    if (tbl->total_width > 0)
		w = tbl->total_width;
	    else if (width > 0)
		w = width;
	}
	tok = process_img(tag, w);
	feed_table1(tbl, tok, mode, width);
	break;
    case HTML_FORM:
	feed_table_block_tag(tbl, "", mode, 0, cmd);
	tmp = process_form(tag);
	if (tmp)
	    feed_table1(tbl, tmp, mode, width);
	break;
    case HTML_N_FORM:
	feed_table_block_tag(tbl, "", mode, 0, cmd);
	process_n_form();
	break;
    case HTML_INPUT:
	tmp = process_input(tag);
	feed_table1(tbl, tmp, mode, width);
	break;
    case HTML_BUTTON:
       tmp = process_button(tag);
       feed_table1(tbl, tmp, mode, width);
       break;
    case HTML_N_BUTTON:
       tmp = process_n_button();
       feed_table1(tbl, tmp, mode, width);
       break;
    case HTML_SELECT:
	tmp = process_select(tag);
	if (tmp)
	    feed_table1(tbl, tmp, mode, width);
	mode->pre_mode |= TBLM_INSELECT;
	mode->end_tag = HTML_N_SELECT;
	break;
    case HTML_N_SELECT:
    case HTML_OPTION:
	/* nothing */
	break;
    case HTML_TEXTAREA:
	w = 0;
	check_rowcol(tbl, mode);
	if (tbl->col + 1 <= tbl->maxcol &&
	    tbl->tabattr[tbl->row][tbl->col + 1] & HTT_X) {
	    if (cell->icell >= 0 && cell->fixed_width[cell->icell] > 0)
		w = cell->fixed_width[cell->icell];
	}
	else {
	    if (tbl->fixed_width[tbl->col] > 0)
		w = tbl->fixed_width[tbl->col];
	}
	tmp = process_textarea(tag, w);
	if (tmp)
	    feed_table1(tbl, tmp, mode, width);
	mode->pre_mode |= TBLM_INTXTA;
	mode->end_tag = HTML_N_TEXTAREA;
	break;
    case HTML_A:
	table_close_anchor0(tbl, mode);
	anchor = NULL;
	i = 0;
	parsedtag_get_value(tag, ATTR_HREF, &anchor);
	parsedtag_get_value(tag, ATTR_HSEQ, &i);
	if (anchor) {
	    check_rowcol(tbl, mode);
	    if (i == 0) {
		Str tmp = process_anchor(tag, line);
    		if (displayLinkNumber)
		{
			Str t = getLinkNumberStr(-1);
			feed_table_inline_tag(tbl, NULL, mode, t->length);
			Strcat(tmp, t);
		}
		pushdata(tbl, tbl->row, tbl->col, tmp->ptr);
	    }
	    else
		pushdata(tbl, tbl->row, tbl->col, line);
	    if (i >= 0) {
		mode->pre_mode |= TBLM_ANCHOR;
		mode->anchor_offset = tbl->tabcontentssize;
	    }
	}
	else
	    suspend_or_pushdata(tbl, line);
	break;
    case HTML_DEL:
	switch (displayInsDel) {
	case DISPLAY_INS_DEL_SIMPLE:
	    mode->pre_mode |= TBLM_DEL;
	    break;
	case DISPLAY_INS_DEL_NORMAL:
	    feed_table_inline_tag(tbl, line, mode, 5);	/* [DEL: */
	    break;
	case DISPLAY_INS_DEL_FONTIFY:
	    feed_table_inline_tag(tbl, line, mode, -1);
	    break;
	}
	break;
    case HTML_N_DEL:
	switch (displayInsDel) {
	case DISPLAY_INS_DEL_SIMPLE:
	    mode->pre_mode &= ~TBLM_DEL;
	    break;
	case DISPLAY_INS_DEL_NORMAL:
	    feed_table_inline_tag(tbl, line, mode, 5);	/* :DEL] */
	    break;
	case DISPLAY_INS_DEL_FONTIFY:
	    feed_table_inline_tag(tbl, line, mode, -1);
	    break;
	}
	break;
    case HTML_S:
	switch (displayInsDel) {
	case DISPLAY_INS_DEL_SIMPLE:
	    mode->pre_mode |= TBLM_S;
	    break;
	case DISPLAY_INS_DEL_NORMAL:
	    feed_table_inline_tag(tbl, line, mode, 3);	/* [S: */
	    break;
	case DISPLAY_INS_DEL_FONTIFY:
	    feed_table_inline_tag(tbl, line, mode, -1);
	    break;
	}
	break;
    case HTML_N_S:
	switch (displayInsDel) {
	case DISPLAY_INS_DEL_SIMPLE:
	    mode->pre_mode &= ~TBLM_S;
	    break;
	case DISPLAY_INS_DEL_NORMAL:
	    feed_table_inline_tag(tbl, line, mode, 3);	/* :S] */
	    break;
	case DISPLAY_INS_DEL_FONTIFY:
	    feed_table_inline_tag(tbl, line, mode, -1);
	    break;
	}
	break;
    case HTML_INS:
    case HTML_N_INS:
	switch (displayInsDel) {
	case DISPLAY_INS_DEL_SIMPLE:
	    break;
	case DISPLAY_INS_DEL_NORMAL:
	    feed_table_inline_tag(tbl, line, mode, 5);	/* [INS:, :INS] */
	    break;
	case DISPLAY_INS_DEL_FONTIFY:
	    feed_table_inline_tag(tbl, line, mode, -1);
	    break;
	}
	break;
    case HTML_SUP:
    case HTML_SUB:
    case HTML_N_SUB:
	if (!(mode->pre_mode & (TBLM_DEL | TBLM_S)))
	    feed_table_inline_tag(tbl, line, mode, 1);	/* ^, [, ] */
	break;
    case HTML_N_SUP:
	break;
    case HTML_TABLE_ALT:
	id = -1;
	parsedtag_get_value(tag, ATTR_TID, &id);
	if (id >= 0 && id < tbl->ntable) {
	    struct table *tbl1 = tbl->tables[id].ptr;
	    feed_table_block_tag(tbl, line, mode, 0, cmd);
	    addcontentssize(tbl, maximum_table_width(tbl1));
	    check_minimum0(tbl, tbl1->sloppy_width);
#ifdef TABLE_EXPAND
	    w = tbl1->total_width;
	    v = 0;
	    colspan = table_colspan(tbl, tbl->row, tbl->col);
	    if (colspan > 1) {
		if (cell->icell >= 0)
		    v = cell->fixed_width[cell->icell];
	    }
	    else
		v = tbl->fixed_width[tbl->col];
	    if (v < 0 && tbl->real_width > 0 && tbl1->real_width > 0)
		w = -(tbl1->real_width * 100) / tbl->real_width;
	    else
		w = tbl1->real_width;
	    if (w > 0)
		check_minimum0(tbl, w);
	    else if (w < 0 && v < w) {
		if (colspan > 1) {
		    if (cell->icell >= 0)
			cell->fixed_width[cell->icell] = w;
		}
		else
		    tbl->fixed_width[tbl->col] = w;
	    }
#endif
	    setwidth0(tbl, mode);
	    clearcontentssize(tbl, mode);
	}
	break;
    case HTML_CAPTION:
	mode->caption = 1;
	break;
    case HTML_N_CAPTION:
    case HTML_THEAD:
    case HTML_N_THEAD:
    case HTML_TBODY:
    case HTML_N_TBODY:
    case HTML_TFOOT:
    case HTML_N_TFOOT:
    case HTML_COLGROUP:
    case HTML_N_COLGROUP:
    case HTML_COL:
	break;
    case HTML_SCRIPT:
	mode->pre_mode |= TBLM_SCRIPT;
	mode->end_tag = HTML_N_SCRIPT;
	break;
    case HTML_STYLE:
	mode->pre_mode |= TBLM_STYLE;
	mode->end_tag = HTML_N_STYLE;
	break;
    case HTML_N_A:
	table_close_anchor0(tbl, mode);
    case HTML_FONT:
    case HTML_N_FONT:
    case HTML_NOP:
	suspend_or_pushdata(tbl, line);
	break;
    case HTML_INTERNAL:
    case HTML_N_INTERNAL:
    case HTML_FORM_INT:
    case HTML_N_FORM_INT:
    case HTML_INPUT_ALT:
    case HTML_N_INPUT_ALT:
    case HTML_SELECT_INT:
    case HTML_N_SELECT_INT:
    case HTML_OPTION_INT:
    case HTML_TEXTAREA_INT:
    case HTML_N_TEXTAREA_INT:
    case HTML_IMG_ALT:
    case HTML_SYMBOL:
    case HTML_N_SYMBOL:
    default:
	/* unknown tag: put into table */
	return TAG_ACTION_FEED;
    }
    return TAG_ACTION_NONE;
}