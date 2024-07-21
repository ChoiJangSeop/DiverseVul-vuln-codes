txtwrite_process_cmap_text(gs_text_enum_t *pte)
{
    textw_text_enum_t *const penum = (textw_text_enum_t *)pte;
    unsigned int rcode = 0;
    gs_text_enum_t scan = *(gs_text_enum_t *)pte;

    /* Composite font using a CMap */
    for ( ; ; ) {
        gs_glyph glyph;
        int font_code, code;
        gs_font *subfont;
        gs_char chr;
        txt_glyph_widths_t widths;
        gs_matrix m3;
        gs_point wanted;	/* user space */
        gs_point dpt = {0,0};

        font_code = scan.orig_font->procs.next_char_glyph
                (&scan, &chr, &glyph);

        subfont = scan.fstack.items[scan.fstack.depth].font;

        switch (font_code) {
            case 0:		/* no font change */
            case 1:		/* font change */
                code = txt_glyph_widths(subfont, scan.orig_font->WMode, glyph, (gs_font *)subfont, &widths,
                        penum->cdevproc_callout ? penum->cdevproc_result : NULL);
                if (code == TEXT_PROCESS_CDEVPROC) {
                    penum->cdevproc_callout = true;
                    pte->returned.current_glyph = glyph;
                    scan.returned.current_glyph = glyph;
                    pte->current_font = subfont;
                    scan.current_font = subfont;
                    rcode = TEXT_PROCESS_CDEVPROC;
                    break;
                }
                else {
                    penum->cdevproc_callout = false;
                    pte->index = scan.index;
                }
                code = gs_matrix_multiply(&subfont->FontMatrix, &pte->orig_font->FontMatrix, &m3);
                if (code < 0)
                    return code;
                code = txt_update_text_state(penum->text_state, (textw_text_enum_t *)pte, pte->orig_font, &m3);
                if (code < 0)
                    return code;
                txt_char_widths_to_uts(pte->orig_font, &widths); /* convert design->text space */
                gs_distance_transform(widths.real_width.xy.x * penum->text_state->size,
                          widths.real_width.xy.y * penum->text_state->size,
                          &penum->text_state->matrix, &wanted);
                pte->returned.total_width.x += wanted.x;
                pte->returned.total_width.y += wanted.y;
                penum->Widths[penum->TextBufferIndex] = wanted.x;

                if (pte->text.operation & TEXT_ADD_TO_ALL_WIDTHS) {
                    gs_point tpt;

                    gs_distance_transform(pte->text.delta_all.x, pte->text.delta_all.y,
                              &ctm_only(pte->pgs), &tpt);
                    dpt.x += tpt.x;
                    dpt.y += tpt.y;
                }
                if (pte->text.operation & TEXT_ADD_TO_SPACE_WIDTH && chr == pte->text.space.s_char) {
                    gs_point tpt;

                    gs_distance_transform(pte->text.delta_space.x, pte->text.delta_space.y,
                              &ctm_only(pte->pgs), &tpt);
                    dpt.x += tpt.x;
                    dpt.y += tpt.y;
                }
                pte->returned.total_width.x += dpt.x;
                pte->returned.total_width.y += dpt.y;

                penum->TextBufferIndex += get_unicode(penum, (gs_font *)pte->orig_font, glyph, chr, &penum->TextBuffer[penum->TextBufferIndex]);
                penum->Widths[penum->TextBufferIndex] += dpt.x;
                break;
            case 2:		/* end of string */
                return 0;
            default:	        /* error */
                return font_code;
        }
        if (rcode || pte->index >= pte->text.size)
            break;
    }
    return rcode;
}