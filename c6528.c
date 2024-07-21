ztempfile(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const char *pstr;
    char fmode[4];
    int code = parse_file_access_string(op, fmode);
    char *prefix = NULL;
    char *fname= NULL;
    uint fnlen;
    FILE *sfile;
    stream *s;
    byte *buf, *sbody;

    if (code < 0)
        return code;
    prefix = (char *)gs_alloc_bytes(imemory, gp_file_name_sizeof, "ztempfile(prefix)");
    fname = (char *)gs_alloc_bytes(imemory, gp_file_name_sizeof, "ztempfile(fname)");
    if (!prefix || !fname) {
        code = gs_note_error(gs_error_VMerror);
        goto done;
    }

    strcat(fmode, gp_fmode_binary_suffix);
    if (r_has_type(op - 1, t_null))
        pstr = gp_scratch_file_name_prefix;
    else {
        uint psize;

        check_read_type(op[-1], t_string);
        psize = r_size(op - 1);
        if (psize >= gp_file_name_sizeof) {
            code = gs_note_error(gs_error_rangecheck);
            goto done;
        }
        memcpy(prefix, op[-1].value.const_bytes, psize);
        prefix[psize] = 0;
        pstr = prefix;
    }

    if (gp_file_name_is_absolute(pstr, strlen(pstr))) {
        if (check_file_permissions(i_ctx_p, pstr, strlen(pstr),
                                   NULL, "PermitFileWriting") < 0) {
            code = gs_note_error(gs_error_invalidfileaccess);
            goto done;
        }
    } else if (!prefix_is_simple(pstr)) {
        code = gs_note_error(gs_error_invalidfileaccess);
        goto done;
    }

    s = file_alloc_stream(imemory, "ztempfile(stream)");
    if (s == 0) {
        code = gs_note_error(gs_error_VMerror);
        goto done;
    }
    buf = gs_alloc_bytes(imemory, file_default_buffer_size,
                         "ztempfile(buffer)");
    if (buf == 0) {
        code = gs_note_error(gs_error_VMerror);
        goto done;
    }
    sfile = gp_open_scratch_file(imemory, pstr, fname, fmode);
    if (sfile == 0) {
        gs_free_object(imemory, buf, "ztempfile(buffer)");
        code = gs_note_error(gs_error_invalidfileaccess);
        goto done;
    }
    fnlen = strlen(fname);
    sbody = ialloc_string(fnlen, ".tempfile(fname)");
    if (sbody == 0) {
        gs_free_object(imemory, buf, "ztempfile(buffer)");
        code = gs_note_error(gs_error_VMerror);
        goto done;
    }
    memcpy(sbody, fname, fnlen);
    file_init_stream(s, sfile, fmode, buf, file_default_buffer_size);
    code = ssetfilename(s, (const unsigned char*) fname, fnlen);
    if (code < 0) {
        gx_io_device *iodev_dflt = iodev_default(imemory);
        sclose(s);
        iodev_dflt->procs.delete_file(iodev_dflt, fname);
        ifree_string(sbody, fnlen, ".tempfile(fname)");
        code = gs_note_error(gs_error_VMerror);
        goto done;
    }
    make_string(op - 1, a_readonly | icurrent_space, fnlen, sbody);
    make_stream_file(op, s, fmode);

done:
    if (prefix)
        gs_free_object(imemory, prefix, "ztempfile(prefix)");
    if (fname)
        gs_free_object(imemory, fname, "ztempfile(fname)");
    return code;
}