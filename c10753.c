mswin_printer_fopen(gx_io_device * iodev, const char *fname, const char *access,
                    gp_file ** pfile, char *rfname, uint rnamelen, gs_memory_t *mem)
{
    DWORD version = GetVersion();
    HANDLE hprinter;
    int pipeh[2];
    uintptr_t tid;
    HANDLE hthread;
    char pname[gp_file_name_sizeof];
    uintptr_t *ptid = &((tid_t *)(iodev->state))->tid;
    gs_lib_ctx_t *ctx = mem->gs_lib_ctx;
    gs_fs_list_t *fs = ctx->core->fs;

    if (gp_validate_path(mem, fname, access) != 0)
        return gs_error_invalidfileaccess;

    /* First we try the open_printer method. */
    /* Note that the loop condition here ensures we don't
     * trigger on the last registered fs entry (our standard
     * 'file' one). */
    if (access[0] == 'w' || access[0] == 'a')
    {
        *pfile = NULL;
        for (fs = ctx->core->fs; fs != NULL && fs->next != NULL; fs = fs->next)
        {
            int code = 0;
            if (fs->fs.open_printer)
                code = fs->fs.open_printer(mem, fs->secret, fname, 1, pfile);
            if (code < 0)
                return code;
            if (*pfile != NULL)
                return code;
        }
    } else
        return gs_error_invalidfileaccess;

    /* If nothing claimed that, then continue with the
     * standard MS way of working. */

    /* Win32s supports neither pipes nor Win32 printers. */
    if (((HIWORD(version) & 0x8000) != 0) &&
        ((HIWORD(version) & 0x4000) == 0))
        return_error(gs_error_invalidfileaccess);

    /* Make sure that printer exists. */
    if (!gp_OpenPrinter((LPTSTR)fname, &hprinter))
        return_error(gs_error_invalidfileaccess);
    ClosePrinter(hprinter);

    *pfile = gp_file_FILE_alloc(mem);
    if (*pfile == NULL)
        return_error(gs_error_VMerror);

    /* Create a pipe to connect a FILE pointer to a Windows printer. */
    if (_pipe(pipeh, 4096, _O_BINARY) != 0) {
        gp_file_dealloc(*pfile);
        *pfile = NULL;
        return_error(gs_fopen_errno_to_code(errno));
    }

    if (gp_file_FILE_set(*pfile, fdopen(pipeh[1], (char *)access), NULL)) {
        *pfile = NULL;
        close(pipeh[0]);
        close(pipeh[1]);
        return_error(gs_fopen_errno_to_code(errno));
    }

    /* start a thread to read the pipe */
    tid = _beginthread(&mswin_printer_thread, 32768, (void *)(intptr_t)(pipeh[0]));
    if (tid == -1) {
        gp_fclose(*pfile);
        *pfile = NULL;
        close(pipeh[0]);
        return_error(gs_error_invalidfileaccess);
    }
    /* Duplicate thread handle so we can wait on it
     * even if original handle is closed by CRTL
     * when the thread finishes.
     */
    if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)tid,
        GetCurrentProcess(), &hthread,
        0, FALSE, DUPLICATE_SAME_ACCESS)) {
        gp_fclose(*pfile);
        *pfile = NULL;
        return_error(gs_error_invalidfileaccess);
    }
    *ptid = (uintptr_t)hthread;

    /* Give the name of the printer to the thread by writing
     * it to the pipe.  This is avoids elaborate thread
     * synchronisation code.
     */
    strncpy(pname, fname, sizeof(pname));
    gp_fwrite(pname, 1, sizeof(pname), *pfile);

    return 0;
}