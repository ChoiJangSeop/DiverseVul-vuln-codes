lib_file_open(gs_file_path_ptr  lib_path, const gs_memory_t *mem, i_ctx_t *i_ctx_p,
                       const char *fname, uint flen, char *buffer, int blen, uint *pclen, ref *pfile)
{   /* i_ctx_p is NULL running arg (@) files.
     * lib_path and mem are never NULL
     */
    bool starting_arg_file = (i_ctx_p == NULL) ? true : i_ctx_p->starting_arg_file;
    bool search_with_no_combine = false;
    bool search_with_combine = false;
    char fmode[4] = { 'r', 0, 0, 0 };           /* room for binary suffix */
    gx_io_device *iodev = iodev_default(mem);
    gs_main_instance *minst = get_minst_from_memory(mem);
    int code;

    /* Once we've opened the arg file, prevent searching current dir from now on */
    /* If this causes problems with "strange" scripts and utlities, we have to rethink */
    if (i_ctx_p)
        i_ctx_p->starting_arg_file = false;

    /* when starting arg files (@ files) iodev_default is not yet set */
    if (iodev == 0)
        iodev = (gx_io_device *)gx_io_device_table[0];

    strcat(fmode, gp_fmode_binary_suffix);
    if (gp_file_name_is_absolute(fname, flen)) {
       search_with_no_combine = true;
       search_with_combine = false;
    } else {
       search_with_no_combine = starting_arg_file;
       search_with_combine = true;
    }
    if (minst->search_here_first) {
      if (search_with_no_combine) {
        code = lib_file_open_search_with_no_combine(lib_path, mem, i_ctx_p,
                                                    fname, flen, buffer, blen, pclen, pfile,
                                                    iodev, starting_arg_file, fmode);
        if (code <= 0) /* +ve means continue continue */
          return code;
      }
      if (search_with_combine) {
        code = lib_file_open_search_with_combine(lib_path, mem, i_ctx_p,
                                                 fname, flen, buffer, blen, pclen, pfile,
                                                 iodev, starting_arg_file, fmode);
        if (code <= 0) /* +ve means continue searching */
          return code;
      }
    } else {
      if (search_with_combine) {
        code = lib_file_open_search_with_combine(lib_path, mem, i_ctx_p,
                                                 fname, flen, buffer, blen, pclen, pfile,
                                                 iodev, starting_arg_file, fmode);
        if (code <= 0) /* +ve means continue searching */
          return code;
      }
      if (search_with_no_combine) {
        code = lib_file_open_search_with_no_combine(lib_path, mem, i_ctx_p,
                                                    fname, flen, buffer, blen, pclen, pfile,
                                                    iodev, starting_arg_file, fmode);
        if (code <= 0) /* +ve means continue searching */
          return code;
      }
    }
    return_error(e_undefinedfilename);
}