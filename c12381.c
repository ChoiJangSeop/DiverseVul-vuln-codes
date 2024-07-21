int wolfSSH_SFTP_RecvOpen(WOLFSSH* ssh, int reqId, byte* data, word32 maxSz)
#ifndef USE_WINDOWS_API
{
    WS_SFTP_FILEATRB atr;
    WFD    fd;
    word32 sz;
    char*  dir;
    word32 reason;
    word32 idx = 0;
    int m = 0;
    int ret = WS_SUCCESS;

    word32 outSz = sizeof(WFD) + UINT32_SZ + WOLFSSH_SFTP_HEADER;
    byte*  out = NULL;

    char* res   = NULL;
    char  ier[] = "Internal Failure";
    char  oer[] = "Open File Error";

    if (ssh == NULL) {
        return WS_BAD_ARGUMENT;
    }

    WLOG(WS_LOG_SFTP, "Receiving WOLFSSH_FTP_OPEN");

    if (sizeof(WFD) > WOLFSSH_MAX_HANDLE) {
        WLOG(WS_LOG_SFTP, "Handle size is too large");
        return WS_FATAL_ERROR;
    }

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

    /* get reason for opening file */
    ato32(data + idx, &reason); idx += UINT32_SZ;

    /* @TODO handle attributes */
    SFTP_ParseAtributes_buffer(ssh, &atr, data, &idx, maxSz);
    if ((reason & WOLFSSH_FXF_READ) && (reason & WOLFSSH_FXF_WRITE)) {
        m |= WOLFSSH_O_RDWR;
    }
    else {
        if (reason & WOLFSSH_FXF_READ) {
            m |= WOLFSSH_O_RDONLY;
        }
        if (reason & WOLFSSH_FXF_WRITE) {
            m |= WOLFSSH_O_WRONLY;
        }
    }

    if (reason & WOLFSSH_FXF_APPEND) {
        m |= WOLFSSH_O_APPEND;
    }
    if (reason & WOLFSSH_FXF_CREAT) {
        m |= WOLFSSH_O_CREAT;
    }
    if (reason & WOLFSSH_FXF_TRUNC) {
        m |= WOLFSSH_O_TRUNC;
    }
    if (reason & WOLFSSH_FXF_EXCL) {
        m |= WOLFSSH_O_EXCL;
    }

    /* if file permissions not set then use default */
    if (!(atr.flags & WOLFSSH_FILEATRB_PERM)) {
        atr.per = 0644;
    }

    if (wolfSSH_CleanPath(ssh, dir) < 0) {
        WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);
        return WS_FATAL_ERROR;
    }
    fd = WOPEN(dir, m, atr.per);
    if (fd < 0) {
        WLOG(WS_LOG_SFTP, "Error opening file %s", dir);
        res = oer;
        if (wolfSSH_SFTP_CreateStatus(ssh, WOLFSSH_FTP_FAILURE, reqId, res,
                "English", NULL, &outSz) != WS_SIZE_ONLY) {
            WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);
            return WS_FATAL_ERROR;
        }
        ret = WS_BAD_FILE_E;
    }

#ifdef WOLFSSH_STOREHANDLE
    if (ret == WS_SUCCESS) {
        if ((ret = SFTP_AddHandleNode(ssh, (byte*)&fd, sizeof(WFD), dir)) != WS_SUCCESS) {
            WLOG(WS_LOG_SFTP, "Unable to store handle");
            res = ier;
            if (wolfSSH_SFTP_CreateStatus(ssh, WOLFSSH_FTP_FAILURE, reqId, res,
                "English", NULL, &outSz) != WS_SIZE_ONLY) {
                WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);
                return WS_FATAL_ERROR;
            }
            ret = WS_FATAL_ERROR;
        }
    }
#endif
    WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);

    /* create packet */
    out = (byte*)WMALLOC(outSz, ssh->ctx->heap, DYNTYPE_BUFFER);
    if (out == NULL) {
        return WS_MEMORY_E;
    }
    if (ret == WS_SUCCESS) {
        if (SFTP_CreatePacket(ssh, WOLFSSH_FTP_HANDLE, out, outSz,
            (byte*)&fd, sizeof(WFD)) != WS_SUCCESS) {
            return WS_FATAL_ERROR;
        }
    }
    else {
        if (wolfSSH_SFTP_CreateStatus(ssh, WOLFSSH_FTP_FAILURE, reqId, res,
                "English", out, &outSz) != WS_SUCCESS) {
            WFREE(out, ssh->ctx->heap, DYNTYPE_BUFFER);
            return WS_FATAL_ERROR;
        }
    }

    /* set send out buffer, "out" is taken by ssh  */
    wolfSSH_SFTP_RecvSetSend(ssh, out, outSz);

    (void)ier;
    return ret;
}