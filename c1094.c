runarg(gs_main_instance * minst, const char *pre, const char *arg,
       const char *post, int options)
{
    int len = strlen(pre) + esc_strlen(arg) + strlen(post) + 1;
    int code;
    char *line;

    if (options & runInit) {
        code = gs_main_init2(minst);    /* Finish initialization */

        if (code < 0)
            return code;
    }
    line = (char *)gs_alloc_bytes(minst->heap, len, "runarg");
    if (line == 0) {
        lprintf("Out of memory!\n");
        return_error(e_VMerror);
    }
    strcpy(line, pre);
    esc_strcat(line, arg);
    strcat(line, post);
    minst->i_ctx_p->starting_arg_file = true;
    code = run_string(minst, line, options);
    minst->i_ctx_p->starting_arg_file = false;
    gs_free_object(minst->heap, line, "runarg");
    return code;
}