int wolfSSH_SFTP_RecvRMDIR(WOLFSSH* ssh, int reqId, byte* data, word32 maxSz)
{
    word32 sz;
    int    ret = 0;
    char*  dir;
    word32 idx = 0;
    byte*  out;
    word32 outSz = 0;
    byte   type;

    char err[] = "Remove Directory Error";
    char suc[] = "Removed Directory";
    char* res  = NULL;

    if (ssh == NULL) {
        return WS_BAD_ARGUMENT;
    }

    WLOG(WS_LOG_SFTP, "Receiving WOLFSSH_FTP_RMDIR");

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

    if (wolfSSH_CleanPath(ssh, dir) < 0) {
        ret = WS_FATAL_ERROR;
    }

    if (ret == 0) {
    #ifndef USE_WINDOWS_API
        ret = WRMDIR(ssh->fs, dir);
    #else /* USE_WINDOWS_API */
        ret = WS_RemoveDirectoryA(dir, ssh->ctx->heap) == 0;
    #endif /* USE_WINDOWS_API */
    }
    WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);

    res  = (ret != 0)? err : suc;
    type = (ret != 0)? WOLFSSH_FTP_FAILURE : WOLFSSH_FTP_OK;
    if (wolfSSH_SFTP_CreateStatus(ssh, type, reqId, res,
                "English", NULL, &outSz) != WS_SIZE_ONLY) {
        return WS_FATAL_ERROR;
    }

    out = (byte*)WMALLOC(outSz, ssh->ctx->heap, DYNTYPE_BUFFER);
    if (out == NULL) {
        return WS_MEMORY_E;
    }

    if (ret != 0) {
        /* @TODO errno holds reason for rmdir failure. Status sent could be
         * better if using errno value to send reason i.e. permissions .. */
        WLOG(WS_LOG_SFTP, "Error removing directory %s", dir);
        ret = WS_BAD_FILE_E;
    }
    else {
        ret = WS_SUCCESS;
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