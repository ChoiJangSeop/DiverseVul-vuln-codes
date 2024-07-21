renderTable(struct table *t, int max_width, struct html_feed_environ *h_env)
{
    int i, j, w, r, h;
    Str renderbuf;
    short new_tabwidth[MAXCOL] = { 0 };
#ifdef MATRIX
    int itr;
    VEC *newwidth;
    MAT *mat, *minv;
    PERM *pivot;
#endif				/* MATRIX */
    int width;
    int rulewidth;
    Str vrulea = NULL, vruleb = NULL, vrulec = NULL;
#ifdef ID_EXT
    Str idtag;
#endif				/* ID_EXT */

    t->total_height = 0;
    if (t->maxcol < 0) {
	make_caption(t, h_env);
	return;
    }

    if (t->sloppy_width > max_width)
	max_width = t->sloppy_width;

    rulewidth = table_rule_width(t);

    max_width -= table_border_width(t);

    if (rulewidth > 1)
	max_width = floor_at_intervals(max_width, rulewidth);

    if (max_width < rulewidth)
	max_width = rulewidth;

    check_maximum_width(t);

#ifdef MATRIX
    if (t->maxcol == 0) {
	if (t->tabwidth[0] > max_width)
	    t->tabwidth[0] = max_width;
	if (t->total_width > 0)
	    t->tabwidth[0] = max_width;
	else if (t->fixed_width[0] > 0)
	    t->tabwidth[0] = t->fixed_width[0];
	if (t->tabwidth[0] < t->minimum_width[0])
	    t->tabwidth[0] = t->minimum_width[0];
    }
    else {
	set_table_matrix(t, max_width);

	itr = 0;
	mat = m_get(t->maxcol + 1, t->maxcol + 1);
	pivot = px_get(t->maxcol + 1);
	newwidth = v_get(t->maxcol + 1);
	minv = m_get(t->maxcol + 1, t->maxcol + 1);
	do {
	    m_copy(t->matrix, mat);
	    LUfactor(mat, pivot);
	    LUsolve(mat, pivot, t->vector, newwidth);
	    LUinverse(mat, pivot, minv);
#ifdef TABLE_DEBUG
	    set_integered_width(t, newwidth->ve, new_tabwidth);
	    fprintf(stderr, "itr=%d\n", itr);
	    fprintf(stderr, "max_width=%d\n", max_width);
	    fprintf(stderr, "minimum : ");
	    for (i = 0; i <= t->maxcol; i++)
		fprintf(stderr, "%2d ", t->minimum_width[i]);
	    fprintf(stderr, "\nfixed : ");
	    for (i = 0; i <= t->maxcol; i++)
		fprintf(stderr, "%2d ", t->fixed_width[i]);
	    fprintf(stderr, "\ndecided : ");
	    for (i = 0; i <= t->maxcol; i++)
		fprintf(stderr, "%2d ", new_tabwidth[i]);
	    fprintf(stderr, "\n");
#endif				/* TABLE_DEBUG */
	    itr++;

	} while (check_table_width(t, newwidth->ve, minv, itr));
	set_integered_width(t, newwidth->ve, new_tabwidth);
	check_minimum_width(t, new_tabwidth);
	v_free(newwidth);
	px_free(pivot);
	m_free(mat);
	m_free(minv);
	m_free(t->matrix);
	v_free(t->vector);
	for (i = 0; i <= t->maxcol; i++) {
	    t->tabwidth[i] = new_tabwidth[i];
	}
    }
#else				/* not MATRIX */
    set_table_width(t, new_tabwidth, max_width);
    for (i = 0; i <= t->maxcol; i++) {
	t->tabwidth[i] = new_tabwidth[i];
    }
#endif				/* not MATRIX */

    check_minimum_width(t, t->tabwidth);
    for (i = 0; i <= t->maxcol; i++)
	t->tabwidth[i] = ceil_at_intervals(t->tabwidth[i], rulewidth);

    renderCoTable(t, h_env->limit);

    for (i = 0; i <= t->maxcol; i++) {
	for (j = 0; j <= t->maxrow; j++) {
	    check_row(t, j);
	    if (t->tabattr[j][i] & HTT_Y)
		continue;
	    do_refill(t, j, i, h_env->limit);
	}
    }

    check_minimum_width(t, t->tabwidth);
    t->total_width = 0;
    for (i = 0; i <= t->maxcol; i++) {
	t->tabwidth[i] = ceil_at_intervals(t->tabwidth[i], rulewidth);
	t->total_width += t->tabwidth[i];
    }

    t->total_width += table_border_width(t);

    check_table_height(t);

    for (i = 0; i <= t->maxcol; i++) {
	for (j = 0; j <= t->maxrow; j++) {
	    TextLineList *l;
	    int k;
	    if ((t->tabattr[j][i] & HTT_Y) ||
		(t->tabattr[j][i] & HTT_TOP) || (t->tabdata[j][i] == NULL))
		continue;
	    h = t->tabheight[j];
	    for (k = j + 1; k <= t->maxrow; k++) {
		if (!(t->tabattr[k][i] & HTT_Y))
		    break;
		h += t->tabheight[k];
		switch (t->border_mode) {
		case BORDER_THIN:
		case BORDER_THICK:
		case BORDER_NOWIN:
		    h += 1;
		    break;
		}
	    }
	    h -= t->tabdata[j][i]->nitem;
	    if (t->tabattr[j][i] & HTT_MIDDLE)
		h /= 2;
	    if (h <= 0)
		continue;
	    l = newTextLineList();
	    for (k = 0; k < h; k++)
		pushTextLine(l, newTextLine(NULL, 0));
	    t->tabdata[j][i] = appendGeneralList((GeneralList *)l,
						 t->tabdata[j][i]);
	}
    }

    /* table output */
    width = t->total_width;

    make_caption(t, h_env);

    HTMLlineproc1("<pre for_table>", h_env);
#ifdef ID_EXT
    if (t->id != NULL) {
	idtag = Sprintf("<_id id=\"%s\">", html_quote((t->id)->ptr));
	HTMLlineproc1(idtag->ptr, h_env);
    }
#endif				/* ID_EXT */
    switch (t->border_mode) {
    case BORDER_THIN:
    case BORDER_THICK:
	renderbuf = Strnew();
	print_sep(t, -1, T_TOP, t->maxcol, renderbuf);
	push_render_image(renderbuf, width, t->total_width, h_env);
	t->total_height += 1;
	break;
    }
    vruleb = Strnew();
    switch (t->border_mode) {
    case BORDER_THIN:
    case BORDER_THICK:
	vrulea = Strnew();
	vrulec = Strnew();
	push_symbol(vrulea, TK_VERTICALBAR(t->border_mode), symbol_width, 1);
	for (i = 0; i < t->cellpadding; i++) {
	    Strcat_char(vrulea, ' ');
	    Strcat_char(vruleb, ' ');
	    Strcat_char(vrulec, ' ');
	}
	push_symbol(vrulec, TK_VERTICALBAR(t->border_mode), symbol_width, 1);
    case BORDER_NOWIN:
	push_symbol(vruleb, TK_VERTICALBAR(BORDER_THIN), symbol_width, 1);
	for (i = 0; i < t->cellpadding; i++)
	    Strcat_char(vruleb, ' ');
	break;
    case BORDER_NONE:
	for (i = 0; i < t->cellspacing; i++)
	    Strcat_char(vruleb, ' ');
    }

    for (r = 0; r <= t->maxrow; r++) {
	for (h = 0; h < t->tabheight[r]; h++) {
	    renderbuf = Strnew();
	    if (t->border_mode == BORDER_THIN
		|| t->border_mode == BORDER_THICK)
		Strcat(renderbuf, vrulea);
#ifdef ID_EXT
	    if (t->tridvalue[r] != NULL && h == 0) {
		idtag = Sprintf("<_id id=\"%s\">",
				html_quote((t->tridvalue[r])->ptr));
		Strcat(renderbuf, idtag);
	    }
#endif				/* ID_EXT */
	    for (i = 0; i <= t->maxcol; i++) {
		check_row(t, r);
#ifdef ID_EXT
		if (t->tabidvalue[r][i] != NULL && h == 0) {
		    idtag = Sprintf("<_id id=\"%s\">",
				    html_quote((t->tabidvalue[r][i])->ptr));
		    Strcat(renderbuf, idtag);
		}
#endif				/* ID_EXT */
		if (!(t->tabattr[r][i] & HTT_X)) {
		    w = t->tabwidth[i];
		    for (j = i + 1;
			 j <= t->maxcol && (t->tabattr[r][j] & HTT_X); j++)
			w += t->tabwidth[j] + t->cellspacing;
		    if (t->tabattr[r][i] & HTT_Y) {
			for (j = r - 1; j >= 0 && t->tabattr[j]
			     && (t->tabattr[j][i] & HTT_Y); j--) ;
			print_item(t, j, i, w, renderbuf);
		    }
		    else
			print_item(t, r, i, w, renderbuf);
		}
		if (i < t->maxcol && !(t->tabattr[r][i + 1] & HTT_X))
		    Strcat(renderbuf, vruleb);
	    }
	    switch (t->border_mode) {
	    case BORDER_THIN:
	    case BORDER_THICK:
		Strcat(renderbuf, vrulec);
		t->total_height += 1;
		break;
	    }
	    push_render_image(renderbuf, width, t->total_width, h_env);
	}
	if (r < t->maxrow && t->border_mode != BORDER_NONE) {
	    renderbuf = Strnew();
	    print_sep(t, r, T_MIDDLE, t->maxcol, renderbuf);
	    push_render_image(renderbuf, width, t->total_width, h_env);
	}
	t->total_height += t->tabheight[r];
    }
    switch (t->border_mode) {
    case BORDER_THIN:
    case BORDER_THICK:
	renderbuf = Strnew();
	print_sep(t, t->maxrow, T_BOTTOM, t->maxcol, renderbuf);
	push_render_image(renderbuf, width, t->total_width, h_env);
	t->total_height += 1;
	break;
    }
    if (t->total_height == 0) {
       renderbuf = Strnew_charp(" ");
	t->total_height++;
	t->total_width = 1;
	push_render_image(renderbuf, 1, t->total_width, h_env);
    }
    HTMLlineproc1("</pre>", h_env);
}