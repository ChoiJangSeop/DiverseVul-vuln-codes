pipe_fopen(gx_io_device * iodev, const char *fname, const char *access,
           gp_file ** pfile, char *rfname, uint rnamelen, gs_memory_t *mem)
{
#ifdef GS_NO_FILESYSTEM
    return 0;
#else
    gs_lib_ctx_t *ctx = mem->gs_lib_ctx;
    gs_fs_list_t *fs = ctx->core->fs;

    if (gp_validate_path(mem, fname, access) != 0)
        return gs_error_invalidfileaccess;

    /*
     * Some platforms allow opening a pipe with a '+' in the access
     * mode, even though pipes are not positionable.  Detect this here.
     */
    if (strchr(access, '+'))
        return_error(gs_error_invalidfileaccess);

    *pfile = NULL;
    for (fs = ctx->core->fs; fs != NULL; fs = fs->next)
    {
        int code = 0;
        if (fs->fs.open_pipe)
            code = fs->fs.open_pipe(mem, fs->secret, fname, rfname, access, pfile);
        if (code < 0)
            return code;
        if (*pfile != NULL)
            break;
    }

    return 0;
#endif
}