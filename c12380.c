int wolfSSH_SFTP_RecvMKDIR(WOLFSSH* ssh, int reqId, byte* data, word32 maxSz)
{
    word32 sz;
    int    ret;
    char*  dir;
    word32 mode = 0;
    word32 idx  = 0;
    byte*  out;
    word32 outSz = 0;
    byte   type;

    char err[] = "Create Directory Error";
    char suc[] = "Created Directory";
    char* res  = NULL;


    if (ssh == NULL) {
        return WS_BAD_ARGUMENT;
    }

    WLOG(WS_LOG_SFTP, "Receiving WOLFSSH_FTP_MKDIR");

    ato32(data + idx, &sz); idx += UINT32_SZ;
    if (sz + idx > maxSz) {
        return WS_BUFFER_E;
    }

    /* plus one to make sure is null terminated */
    dir = (char*)WMALLOC(sz + 1, ssh->ctx->heap, DYNTYPE_BUFFER);
    if (dir == NULL) {
        return WS_MEMORY_E;
    }
    WMEMCPY(dir, data + idx, sz);
    dir[sz] = '\0';
    idx += sz;
    if (idx + UINT32_SZ > maxSz) {
        WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);
        return WS_BUFFER_E;
    }

    ato32(data + idx, &sz); idx += UINT32_SZ;
    if (idx + sz > maxSz) {
        WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);
        return WS_BUFFER_E;
    }
    if (sz != UINT32_SZ) {
        WLOG(WS_LOG_SFTP, "Attribute size larger than 4 not yet supported");
        WLOG(WS_LOG_SFTP, "Skipping over attribute and using default");
        mode = 0x41ED;
    }
    else {
        ato32(data + idx, &mode);
    }

    if (wolfSSH_CleanPath(ssh, dir) < 0) {
        WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);
        return WS_FATAL_ERROR;
    }

#ifndef USE_WINDOWS_API
    ret = WMKDIR(ssh->fs, dir, mode);
#else /* USE_WINDOWS_API */
    ret = WS_CreateDirectoryA(dir, ssh->ctx->heap) == 0;
#endif /* USE_WINDOWS_API */
    WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);

    res  = (ret != 0)? err : suc;
    type = (ret != 0)? WOLFSSH_FTP_FAILURE : WOLFSSH_FTP_OK;
    if (ret != 0) {
        WLOG(WS_LOG_SFTP, "Error creating directory %s", dir);
        ret  = WS_BAD_FILE_E;
    }
    else {
        ret  = WS_SUCCESS;
    }

    if (wolfSSH_SFTP_CreateStatus(ssh, type, reqId, res,
                "English", NULL, &outSz) != WS_SIZE_ONLY) {
        return WS_FATAL_ERROR;
    }
    out = (byte*)WMALLOC(outSz, ssh->ctx->heap, DYNTYPE_BUFFER);
    if (out == NULL) {
        return WS_MEMORY_E;
    }

    if (wolfSSH_SFTP_CreateStatus(ssh, type, reqId, res, "English", out,
                &outSz) != WS_SUCCESS) {
        WFREE(out, ssh->ctx->heap, DYNTYPE_BUFFER);
        return WS_FATAL_ERROR;
    }

    /* set send out buffer, "out" is taken by ssh  */
    wolfSSH_SFTP_RecvSetSend(ssh, out, outSz);
    return ret;
}