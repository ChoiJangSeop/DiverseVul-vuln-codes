context_state_alloc(gs_context_state_t ** ppcst,
                    const ref *psystem_dict,
                    const gs_dual_memory_t * dmem)
{
    gs_ref_memory_t *mem = dmem->space_local;
    gs_context_state_t *pcst = *ppcst;
    int code;
    int i;

    if (pcst == 0) {
        pcst = gs_alloc_struct((gs_memory_t *) mem, gs_context_state_t,
                               &st_context_state, "context_state_alloc");
        if (pcst == 0)
            return_error(gs_error_VMerror);
    }
    code = gs_interp_alloc_stacks(mem, pcst);
    if (code < 0)
        goto x0;
    /*
     * We have to initialize the dictionary stack early,
     * for far-off references to systemdict.
     */
    pcst->dict_stack.system_dict = *psystem_dict;
    pcst->dict_stack.min_size = 0;
    pcst->dict_stack.userdict_index = 0;
    pcst->pgs = int_gstate_alloc(dmem);
    if (pcst->pgs == 0) {
        code = gs_note_error(gs_error_VMerror);
        goto x1;
    }
    pcst->memory = *dmem;
    pcst->language_level = 1;
    make_false(&pcst->array_packing);
    make_int(&pcst->binary_object_format, 0);
    pcst->nv_page_count = 0;
    pcst->rand_state = rand_state_initial;
    pcst->usertime_inited = false;
    pcst->in_superexec = 0;
    pcst->plugin_list = 0;
    make_t(&pcst->error_object, t__invalid);
    {	/*
         * Create an empty userparams dictionary of the right size.
         * If we can't determine the size, pick an arbitrary one.
         */
        ref *puserparams;
        uint size;
        ref *system_dict = &pcst->dict_stack.system_dict;

        if (dict_find_string(system_dict, "userparams", &puserparams) > 0)
            size = dict_length(puserparams);
        else
            size = 300;
        code = dict_alloc(pcst->memory.space_local, size, &pcst->userparams);
        if (code < 0)
            goto x2;
        /* PostScript code initializes the user parameters. */
    }
    pcst->scanner_options = 0;
    pcst->LockFilePermissions = false;
    pcst->starting_arg_file = false;
    pcst->RenderTTNotdef = true;

    pcst->invalid_file_stream = (stream*)gs_alloc_struct_immovable(mem->stable_memory,
                                          stream, &st_stream,
                                          "context_state_alloc");
    if (pcst->invalid_file_stream == NULL) {
        code = gs_note_error(gs_error_VMerror);
        goto x3;
    }

    s_init(pcst->invalid_file_stream, mem->stable_memory);
    sread_string(pcst->invalid_file_stream, NULL, 0);
    s_init_no_id(pcst->invalid_file_stream);

    /* The initial stdio values are bogus.... */
    make_file(&pcst->stdio[0], a_readonly | avm_invalid_file_entry, 1,
              pcst->invalid_file_stream);
    make_file(&pcst->stdio[1], a_all | avm_invalid_file_entry, 1,
              pcst->invalid_file_stream);
    make_file(&pcst->stdio[2], a_all | avm_invalid_file_entry, 1,
              pcst->invalid_file_stream);
    for (i = countof(pcst->memory.spaces_indexed); --i >= 0;)
        if (dmem->spaces_indexed[i] != 0)
            ++(dmem->spaces_indexed[i]->num_contexts);
    /* The number of interpreter "ticks" between calls on the time_slice_proc.
     * Currently, the clock ticks before each operator, and at each
     * procedure return. */
    pcst->time_slice_ticks = 0x7fff;
    *ppcst = pcst;
    return 0;
  x3:/* No need to delete dictionary here, as gc will do it for us. */
  x2:gs_gstate_free(pcst->pgs);
  x1:gs_interp_free_stacks(mem, pcst);
  x0:if (*ppcst == 0)
        gs_free_object((gs_memory_t *) mem, pcst, "context_state_alloc");
    return code;
}