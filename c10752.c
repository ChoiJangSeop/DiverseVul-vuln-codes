os2_printer_fopen(gx_io_device * iodev, const char *fname, const char *access,
           FILE ** pfile, char *rfname, uint rnamelen)
{
    os2_printer_t *pr = (os2_printer_t *)iodev->state;
    char driver_name[256];
    gs_lib_ctx_t *ctx = mem->gs_lib_ctx;
    gs_fs_list_t *fs = ctx->core->fs;

    /* First we try the open_printer method. */
    /* Note that the loop condition here ensures we don't
     * trigger on the last registered fs entry (out standard
     * file one). */
    *pfile = NULL;
    for (fs = ctx->core->fs; fs != NULL && fs->next != NULL; fs = fs->next)
    {
        int code = 0;
        if (fs->fs.open_printer)
            code = fs->fs.open_printer(mem, fs->secret, fname, access, pfile);
        if (code < 0)
            return code;
        if (*pfile != NULL)
            return code;
    }

    /* If nothing claimed that, then continue with the
     * standard OS/2 way of working. */

    /* Make sure that printer exists. */
    if (pm_find_queue(pr->memory, fname, driver_name)) {
        /* error, list valid queue names */
        emprintf(pr->memory, "Invalid queue name.  Use one of:\n");
        pm_find_queue(pr->memory, NULL, NULL);
        return_error(gs_error_undefinedfilename);
    }

    strncpy(pr->queue, fname, sizeof(pr->queue)-1);

    /* Create a temporary file */
    *pfile = gp_open_scratch_file_impl(pr->memory, "gs", pr->filename, access, 0);
    if (*pfile == NULL)
        return_error(gs_fopen_errno_to_code(errno));

    return 0;
}