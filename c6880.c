build_gs_font(i_ctx_t *i_ctx_p, os_ptr op, gs_font ** ppfont, font_type ftype,
              gs_memory_type_ptr_t pstype, const build_proc_refs * pbuild,
              build_font_options_t options)
{
    ref kname;			/* t_string */
    ref *pftype;
    ref *pencoding = 0;
    bool bitmapwidths;
    int exactsize, inbetweensize, transformedchar;
    int wmode;
    int code;
    gs_font *pfont;
    ref *pfid;
    ref *aop = dict_access_ref(op);
    bool cpsi_mode = gs_currentcpsimode(imemory);

    get_font_name(imemory, &kname, op - 1);
    if (dict_find_string(op, "FontType", &pftype) <= 0 ||
        !r_has_type(pftype, t_integer) ||
        pftype->value.intval != (int)ftype
        )
        return_error(gs_error_invalidfont);
    if (dict_find_string(op, "Encoding", &pencoding) <= 0) {
        if (!(options & bf_Encoding_optional))
            return_error(gs_error_invalidfont);
        pencoding = 0;
    } else {
        if (!r_is_array(pencoding))
            return_error(gs_error_invalidfont);
    }
    if (pencoding) {   /* observed Adobe behavior */
        int count = r_size(pencoding);
        int type = ftype ? t_name : t_integer;
        bool fixit = false;

        while (count--) {
           ref r;
           if ((code = array_get(imemory, pencoding, count, &r)) < 0 ||
             !(r_has_type(&r, type) || r_has_type(&r, t_null))) {
               if (!cpsi_mode && ftype == ft_user_defined) {
                   if (code < 0 || r_has_type(&r, t_null)) {
                       return_error(gs_error_typecheck);
                   }
                   fixit = true;
                   break;
               }
               else {
                   return_error(gs_error_typecheck);
               }
           }
        }

        /* For at least Type 3 fonts, Adobe Distiller will "fix" an Encoding array, as in, for example
         * Bug 692681 where the arrays contain integers rather than names. Once the font is instantiated
         * the integers have been converted to names.
         * It is preferable to to this manipulation here, rather than in Postscript, because we are less
         * restricted by read-only attributes and VM save levels.
         */
        if (fixit) {
            ref penc;
            uint size = 0;
            char buf[32], *bptr;
            avm_space curglob = ialloc_space(idmemory);
            avm_space useglob = r_is_local(pencoding) ? avm_local : avm_global;

            ialloc_set_space(idmemory, useglob);
            
            count = r_size(pencoding);
            if ((code = ialloc_ref_array(&penc, (r_type_attrs(pencoding) & a_readonly), count, "build_gs_font")) < 0)
                 return code;
            
            while (count--) {
               ref r;
               if (array_get(imemory, pencoding, count, &r) < 0){
                   return_error(gs_error_typecheck);
               }
               /* For type 3, we know the Encoding entries must be names */
               if (r_has_type(&r, t_name)){
                   ref_assign(&(penc.value.refs[count]), &r);
               }
               else {
               
                   if ((code = obj_cvs(imemory, &r, (byte *)buf, 32, &size, (const byte **)(&bptr))) < 0) {
                       return(code);
                   }
                   if ((code = name_ref(imemory, (const byte *)bptr, size, &r, true)) < 0)
                        return code;
                   ref_assign(&(penc.value.refs[count]), &r);
               }
            }
            
            if ((code = dict_put_string(osp, "Encoding", &penc, NULL)) < 0)
               return code;
            ialloc_set_space(idmemory, curglob);
        }
    }
    if ((code = dict_int_param(op, "WMode", 0, 1, 0, &wmode)) < 0 ||
        (code = dict_bool_param(op, "BitmapWidths", false, &bitmapwidths)) < 0 ||
        (code = dict_int_param(op, "ExactSize", 0, 2, fbit_use_bitmaps, &exactsize)) < 0 ||
        (code = dict_int_param(op, "InBetweenSize", 0, 2, fbit_use_outlines, &inbetweensize)) < 0 ||
        (code = dict_int_param(op, "TransformedChar", 0, 2, fbit_use_outlines, &transformedchar)) < 0
        )
        return code;
    code = dict_find_string(op, "FID", &pfid);
    if (code > 0 && r_has_type(pfid, t_fontID)) { /* silently ignore invalid FID per CET 13-05.ps */
        /*
         * If this font has a FID entry already, it might be a scaled font
         * made by makefont or scalefont; in a Level 2 environment, it might
         * be an existing font being registered under a second name, or a
         * re-encoded font (which was invalid in Level 1, but dvips did it
         * anyway).
         */
        pfont = r_ptr(pfid, gs_font);
        /*
         * If the following condition is false this is a re-encoded font,
         * or some other questionable situation in which the FID
         * was preserved.  Pretend the FID wasn't there.
         */
        if (obj_eq(pfont->memory, pfont_dict(pfont), op)) {
            if (pfont->base == pfont) {	/* original font */
                if (!level2_enabled)
                    return_error(gs_error_invalidfont);
                *ppfont = pfont;
                return 1;
            } else {		/* This was made by makefont or scalefont. */
                /* Just insert the new name. */
                gs_matrix mat;
                ref fname;			/* t_string */

                code = sub_font_params(imemory, op, &mat, NULL, &fname);
                if (code < 0)
                    return code;
                code = 1;
                copy_font_name(&pfont->font_name, &fname);
                goto set_name;
            }
        }
    }
    /* This is a new font. */
    if (!r_has_attr(aop, a_write))
        return_error(gs_error_invalidaccess);
    {
        ref encoding;

        /*
         * Since add_FID may resize the dictionary and cause
         * pencoding to become invalid, save the Encoding.
         */
        if (pencoding) {
            encoding = *pencoding;
            pencoding = &encoding;
        }
        code = build_gs_sub_font(i_ctx_p, op, &pfont, ftype, pstype,
                                 pbuild, pencoding, op);
        if (code < 0)
            return code;
    }
    pfont->BitmapWidths = bitmapwidths;
    pfont->ExactSize = (fbit_type)exactsize;
    pfont->InBetweenSize = (fbit_type)inbetweensize;
    pfont->TransformedChar = (fbit_type)transformedchar;
    pfont->WMode = wmode;
    pfont->procs.font_info = zfont_info;
    code = 0;
set_name:
    copy_font_name(&pfont->key_name, &kname);
    *ppfont = pfont;
    return code;
}