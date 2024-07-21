static int wolfSSH_SFTP_RecvRealPath(WOLFSSH* ssh, int reqId, byte* data,
        int maxSz)
{
    WS_SFTP_FILEATRB atr;
    char  r[WOLFSSH_MAX_FILENAME];
    word32 rSz;
    word32 lidx = 0;
    word32 i;
    int    ret;
    byte* out;
    word32 outSz = 0;

    WLOG(WS_LOG_SFTP, "Receiving WOLFSSH_FTP_REALPATH");

    if (ssh == NULL) {
        WLOG(WS_LOG_SFTP, "Bad argument passed in");
        return WS_BAD_ARGUMENT;
    }

    if (maxSz < UINT32_SZ) {
        /* not enough for an ato32 call */
        return WS_BUFFER_E;
    }

    ato32(data + lidx, &rSz);
    if (rSz > WOLFSSH_MAX_FILENAME || (int)(rSz + UINT32_SZ) > maxSz) {
        return WS_BUFFER_E;
    }
    lidx += UINT32_SZ;
    WMEMCPY(r, data + lidx, rSz);
    r[rSz] = '\0';

    /* get working directory in the case of receiving non absolute path */
    if (r[0] != '/' && r[1] != ':') {
        char wd[WOLFSSH_MAX_FILENAME];

        WMEMSET(wd, 0, WOLFSSH_MAX_FILENAME);
        if (ssh->sftpDefaultPath) {
            XSTRNCPY(wd, ssh->sftpDefaultPath, WOLFSSH_MAX_FILENAME - 1);
        }
        else {
        #ifndef USE_WINDOWS_API
            if (WGETCWD(ssh->fs, wd, WOLFSSH_MAX_FILENAME) == NULL) {
                WLOG(WS_LOG_SFTP, "Unable to get current working directory");
                if (wolfSSH_SFTP_CreateStatus(ssh, WOLFSSH_FTP_FAILURE, reqId,
                        "Directory error", "English", NULL, &outSz)
                        != WS_SIZE_ONLY) {
                    return WS_FATAL_ERROR;
                }
                out = (byte*) WMALLOC(outSz, ssh->ctx->heap, DYNTYPE_BUFFER);
                if (out == NULL) {
                    return WS_MEMORY_E;
                }
                if (wolfSSH_SFTP_CreateStatus(ssh, WOLFSSH_FTP_FAILURE, reqId,
                        "Directory error", "English", out, &outSz)
                        != WS_SUCCESS) {
                    WFREE(out, ssh->ctx->heap, DYNTYPE_BUFFER);
                    return WS_FATAL_ERROR;
                }
                /* take over control of buffer */
                wolfSSH_SFTP_RecvSetSend(ssh, out, outSz);
                return WS_BAD_FILE_E;
            }
        #endif
        }
        WSTRNCAT(wd, "/", WOLFSSH_MAX_FILENAME);
        WSTRNCAT(wd, r, WOLFSSH_MAX_FILENAME);
        WMEMCPY(r, wd, WOLFSSH_MAX_FILENAME);
    }

    if ((ret = wolfSSH_CleanPath(ssh, r)) < 0) {
        return WS_FATAL_ERROR;
    }
    rSz = (word32)ret;

    /* For real path remove ending case of /.
     * Lots of peers send a '.' wanting a return of the current absolute path
     * not the absolute path + .
     */
    if (r[rSz - 2] == WS_DELIM && r[rSz - 1] == '.') {
        r[rSz - 1] = '\0';
        rSz -= 1;
    }

    /* for real path always send '/' chars */
    for (i = 0; i < rSz; i++) {
        if (r[i] == WS_DELIM) r[i] = '/';
    }
    WLOG(WS_LOG_SFTP, "Real Path Directory = %s", r);

    /* send response */
    outSz = WOLFSSH_SFTP_HEADER + (UINT32_SZ * 3) + (rSz * 2);
    WMEMSET(&atr, 0, sizeof(WS_SFTP_FILEATRB));
    outSz += SFTP_AtributesSz(ssh, &atr);
    lidx = 0;

    /* reuse state buffer if large enough */
    out = (outSz > (word32)maxSz)?
            (byte*)WMALLOC(outSz, ssh->ctx->heap, DYNTYPE_BUFFER) :
            wolfSSH_SFTP_RecvGetData(ssh);
    if (out == NULL) {
        return WS_MEMORY_E;
    }

    SFTP_SetHeader(ssh, reqId, WOLFSSH_FTP_NAME,
            outSz - WOLFSSH_SFTP_HEADER, out);
    lidx += WOLFSSH_SFTP_HEADER;

    /* set number of files */
    c32toa(1, out + lidx); lidx += UINT32_SZ; /* only sending one file name */

    /* set file name size and string */
    c32toa(rSz, out + lidx); lidx += UINT32_SZ;
    WMEMCPY(out + lidx, r, rSz); lidx += rSz;

    /* set long name size and string */
    c32toa(rSz, out + lidx); lidx += UINT32_SZ;
    WMEMCPY(out + lidx, r, rSz); lidx += rSz;

    /* set attributes */
    SFTP_SetAttributes(ssh, out + lidx, outSz - lidx, &atr);

    /* set send out buffer, "out" buffer is taken over by "ssh" */
    wolfSSH_SFTP_RecvSetSend(ssh, out, outSz);
    return WS_SUCCESS;
}