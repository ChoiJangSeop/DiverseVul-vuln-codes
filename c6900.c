static int setindexedspace(i_ctx_t * i_ctx_p, ref *r, int *stage, int *cont, int CIESubst)
{
    ref *pproc = &istate->colorspace[0].procs.special.index_proc;
    int code = 0;
    uint edepth = ref_stack_count(&e_stack);
    ref_colorspace cspace_old;
    ref hival, lookup;
    gs_color_space *pcs;
    gs_color_space *pcs_base;
    gs_color_space_index base_type;

    if (i_ctx_p->language_level < 2)
        return_error(gs_error_undefined);

    *cont = 0;
    if (*stage == 1) {
        *stage = 0;
        return 0;
    }

    cspace_old = istate->colorspace[0];

    pcs_base = gs_currentcolorspace(igs);
    base_type = gs_color_space_get_index(pcs_base);

    code = array_get(imemory, r, 3, &lookup);
    if (code < 0)
        return code;
    code = array_get(imemory, r, 2, &hival);
    if (code < 0)
        return code;
    if (r_has_type(&lookup, t_string)) {
        int num_values = (hival.value.intval + 1) * cs_num_components(pcs_base);
        byte *data_tmp;

        check_read(lookup);
        /*
         * The PDF and PS specifications state that the lookup table must have
         * the exact number of of data bytes needed.  However we have found
         * PDF files from Amyuni with extra data bytes.  Acrobat 6.0 accepts
         * these files without complaint, so we ignore the extra data.
         */
        if (r_size(&lookup) < num_values)
            return_error(gs_error_rangecheck);
        /* If we have a named color profile and the base space is DeviceN or
           Separation use a different set of procedures to ensure the named
           color remapping code is used */
        if (igs->icc_manager->device_named != NULL && 
            (base_type == gs_color_space_index_Separation ||
             base_type == gs_color_space_index_DeviceN))
            pcs = gs_cspace_alloc(imemory, &gs_color_space_type_Indexed_Named);
        else
            pcs = gs_cspace_alloc(imemory, &gs_color_space_type_Indexed);
        if (!pcs) {
            return_error(gs_error_VMerror);
        }
        pcs->base_space = pcs_base;
        rc_increment_cs(pcs_base);

        data_tmp = (byte *) (pcs->params.indexed.lookup.table.data = ialloc_string (lookup.tas.rsize, "setindexedspace"));
        if (!data_tmp) {
            rc_decrement(pcs, "setindexedspace");
            return_error(gs_error_VMerror);
        }

        memcpy(data_tmp, lookup.value.const_bytes, lookup.tas.rsize);

        pcs->params.indexed.lookup.table.size = num_values;
        pcs->params.indexed.use_proc = 0;
        make_null(pproc);
    } else {
        gs_indexed_map *map;

        /*
         * We have to call zcs_begin_map before moving the parameters,
         * since if the color space is a DeviceN or Separation space,
         * the memmove will overwrite its parameters.
         */
        code = zcs_begin_map(i_ctx_p, &map, &lookup, (hival.value.intval + 1),
                             pcs_base, indexed_cont);
        if (code < 0)
            return code;
        if (igs->icc_manager->device_named != NULL &&
            (base_type == gs_color_space_index_Separation ||
             base_type == gs_color_space_index_DeviceN))
            pcs = gs_cspace_alloc(imemory, &gs_color_space_type_Indexed_Named);
        else
            pcs = gs_cspace_alloc(imemory, &gs_color_space_type_Indexed);
        pcs->base_space = pcs_base;
        rc_increment_cs(pcs_base);
        pcs->params.indexed.use_proc = 1;
        *pproc = lookup;
        map->proc.lookup_index = lookup_indexed_map;
        pcs->params.indexed.lookup.map = map;
    }
    pcs->params.indexed.hival = hival.value.intval;
    pcs->params.indexed.n_comps = cs_num_components(pcs_base);
    code = gs_setcolorspace(igs, pcs);
    /* release reference from construction */
    rc_decrement_only_cs(pcs, "setindexedspace");
    if (code < 0) {
        istate->colorspace[0] = cspace_old;
        ref_stack_pop_to(&e_stack, edepth);
        return code;
    }
    *stage = 0;
    if (ref_stack_count(&e_stack) == edepth) {
        return 0;
    } else {
        *cont = 1;
        *stage = 1;
        return o_push_estack; /* installation will load the caches */
    }
}