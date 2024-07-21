zbuildfont0(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_type0_data data;
    ref fdepvector;
    ref *pprefenc;
    gs_font_type0 *pfont;
    font_data *pdata;
    ref save_FID;
    int i;
    int code = 0;

    check_type(*op, t_dictionary);
    {
        ref *pfmaptype;
        ref *pfdepvector;

        if (dict_find_string(op, "FMapType", &pfmaptype) <= 0 ||
            !r_has_type(pfmaptype, t_integer) ||
            pfmaptype->value.intval < (int)fmap_type_min ||
            pfmaptype->value.intval > (int)fmap_type_max ||
            dict_find_string(op, "FDepVector", &pfdepvector) <= 0 ||
            !r_is_array(pfdepvector)
            )
            return_error(gs_error_invalidfont);
        data.FMapType = (fmap_type) pfmaptype->value.intval;
        /*
         * Adding elements below could cause the font dictionary to be
         * resized, which would invalidate pfdepvector.
         */
        fdepvector = *pfdepvector;
    }
    /* Check that every element of the FDepVector is a font. */
    data.fdep_size = r_size(&fdepvector);
    for (i = 0; i < data.fdep_size; i++) {
        ref fdep;
        gs_font *psub;

        array_get(imemory, &fdepvector, i, &fdep);
        if ((code = font_param(&fdep, &psub)) < 0)
            return code;
        /*
         * Check the inheritance rules.  Allowed configurations
         * (paths from root font) are defined by the regular
         * expression:
         *      (shift | double_escape escape* | escape*)
         *        non_modal* non_composite
         */
        if (psub->FontType == ft_composite) {
            const gs_font_type0 *const psub0 = (const gs_font_type0 *)psub;
            fmap_type fmt = psub0->data.FMapType;

            if (fmt == fmap_double_escape ||
                fmt == fmap_shift ||
                (fmt == fmap_escape &&
                 !(data.FMapType == fmap_escape ||
                   data.FMapType == fmap_double_escape))
                )
                return_error(gs_error_invalidfont);
        }
    }
    switch (data.FMapType) {
        case fmap_escape:
        case fmap_double_escape:	/* need EscChar */
            code = ensure_char_entry(i_ctx_p, op, "EscChar", &data.EscChar, 255);
            break;
        case fmap_shift:	/* need ShiftIn & ShiftOut */
            code = ensure_char_entry(i_ctx_p, op, "ShiftIn", &data.ShiftIn, 15);
            if (code >= 0)
                code = ensure_char_entry(i_ctx_p, op, "ShiftOut", &data.ShiftOut, 14);
            break;
        case fmap_SubsVector:	/* need SubsVector */
            {
                ref *psubsvector;
                uint svsize;

                if (dict_find_string(op, "SubsVector", &psubsvector) <= 0 ||
                    !r_has_type(psubsvector, t_string) ||
                    (svsize = r_size(psubsvector)) == 0 ||
                (data.subs_width = (int)*psubsvector->value.bytes + 1) > 4 ||
                    (svsize - 1) % data.subs_width != 0
                    )
                    return_error(gs_error_invalidfont);
                data.subs_size = (svsize - 1) / data.subs_width;
                data.SubsVector.data = psubsvector->value.bytes + 1;
                data.SubsVector.size = svsize - 1;
            } break;
        case fmap_CMap:	/* need CMap */
            code = ztype0_get_cmap(&data.CMap, (const ref *)&fdepvector,
                                   (const ref *)op, imemory);
            break;
        default:
            ;
    }
    if (code < 0)
        return code;
    /*
     * Save the old FID in case we have to back out.
     * build_gs_font will return an error if there is a FID entry
     * but it doesn't reference a valid font.
     */
    {
        ref *pfid;

        if (dict_find_string(op, "FID", &pfid) <= 0)
            make_null(&save_FID);
        else
            save_FID = *pfid;
    }
    {
        build_proc_refs build;

        code = build_proc_name_refs(imemory, &build,
                                    "%Type0BuildChar", "%Type0BuildGlyph");
        if (code < 0)
            return code;
        code = build_gs_font(i_ctx_p, op, (gs_font **) & pfont,
                             ft_composite, &st_gs_font_type0, &build,
                             bf_options_none);
    }
    if (code != 0)
        return code;
    /* Fill in the rest of the basic font data. */
    pfont->procs.init_fstack = gs_type0_init_fstack;
    pfont->procs.define_font = ztype0_define_font;
    pfont->procs.make_font = ztype0_make_font;
    pfont->procs.next_char_glyph = gs_type0_next_char_glyph;
    pfont->procs.decode_glyph = gs_font_map_glyph_to_unicode; /* PDF needs. */
    if (dict_find_string(op, "PrefEnc", &pprefenc) <= 0) {
        ref nul;

        make_null_new(&nul);
        if ((code = idict_put_string(op, "PrefEnc", &nul)) < 0)
            goto fail;
    }
    get_GlyphNames2Unicode(i_ctx_p, (gs_font *)pfont, op);
    /* Fill in the font data */
    pdata = pfont_data(pfont);
    data.encoding_size = r_size(&pdata->Encoding);
    /*
     * Adobe interpreters apparently require that Encoding.size >= subs_size
     * +1 (not sure whether the +1 only applies if the sum of the range
     * sizes is less than the size of the code space).  The gs library
     * doesn't require this -- it only gives an error if a show operation
     * actually would reference beyond the end of the Encoding -- so we
     * check this here rather than in the library.
     */
    if (data.FMapType == fmap_SubsVector) {
        if (data.subs_size >= r_size(&pdata->Encoding)) {
            code = gs_note_error(gs_error_rangecheck);
            goto fail;
        }
    }
    data.Encoding =
        (uint *) ialloc_byte_array(data.encoding_size, sizeof(uint),
                                   "buildfont0(Encoding)");
    if (data.Encoding == 0) {
        code = gs_note_error(gs_error_VMerror);
        goto fail;
    }
    /* Fill in the encoding vector, checking to make sure that */
    /* each element is an integer between 0 and fdep_size-1. */
    for (i = 0; i < data.encoding_size; i++) {
        ref enc;

        array_get(imemory, &pdata->Encoding, i, &enc);
        if (!r_has_type(&enc, t_integer)) {
            code = gs_note_error(gs_error_typecheck);
            goto fail;
        }
        if ((ulong) enc.value.intval >= data.fdep_size) {
            code = gs_note_error(gs_error_rangecheck);
            goto fail;
        }
        data.Encoding[i] = (uint) enc.value.intval;
    }
    data.FDepVector =
        ialloc_struct_array(data.fdep_size, gs_font *,
                            &st_gs_font_ptr_element,
                            "buildfont0(FDepVector)");
    if (data.FDepVector == 0) {
        code = gs_note_error(gs_error_VMerror);
        goto fail;
    }
    for (i = 0; i < data.fdep_size; i++) {
        ref fdep;
        ref *pfid;

        array_get(pfont->memory, &fdepvector, i, &fdep);
        /* The lookup can't fail, because of the pre-check above. */
        dict_find_string(&fdep, "FID", &pfid);
        data.FDepVector[i] = r_ptr(pfid, gs_font);
    }
    pfont->data = data;
    code = define_gs_font(i_ctx_p, (gs_font *) pfont);
    if (code >= 0)
        return code;
fail:
        /*
         * Undo the insertion of the FID entry in the dictionary.  Note that
         * some allocations (Encoding, FDepVector) are not undone.
         */
    if (r_has_type(&save_FID, t_null)) {
        ref rnfid;

        name_enter_string(pfont->memory, "FID", &rnfid);
        idict_undef(op, &rnfid);
    } else
        idict_put_string(op, "FID", &save_FID);
    gs_free_object(pfont->memory, pfont, "buildfont0(font)");
    return code;
}