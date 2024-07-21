int wolfSSH_SFTP_RecvWrite(WOLFSSH* ssh, int reqId, byte* data, word32 maxSz)
#ifndef USE_WINDOWS_API
{
    WFD    fd;
    word32 sz;
    int    ret  = WS_SUCCESS;
    word32 idx  = 0;
    word32 ofst[2] = {0,0};

    word32 outSz = 0;
    byte*  out   = NULL;

    char  suc[] = "Write File Success";
    char  err[] = "Write File Error";
    char* res  = suc;
    byte  type = WOLFSSH_FTP_OK;

    if (ssh == NULL) {
        return WS_BAD_ARGUMENT;
    }

    WLOG(WS_LOG_SFTP, "Receiving WOLFSSH_FTP_WRITE");

    /* get file handle */
    ato32(data + idx, &sz); idx += UINT32_SZ;
    if (sz + idx > maxSz || sz > WOLFSSH_MAX_HANDLE) {
        WLOG(WS_LOG_SFTP, "Error with file handle size");
        res  = err;
        type = WOLFSSH_FTP_FAILURE;
        ret  = WS_BAD_FILE_E;
    }

    if (ret == WS_SUCCESS) {
        WMEMSET((byte*)&fd, 0, sizeof(WFD));
        WMEMCPY((byte*)&fd, data + idx, sz); idx += sz;

        /* get offset into file */
        ato32(data + idx, &ofst[1]); idx += UINT32_SZ;
        ato32(data + idx, &ofst[0]); idx += UINT32_SZ;

        /* get length to be written */
        ato32(data + idx, &sz); idx += UINT32_SZ;

        ret = WPWRITE(fd, data + idx, sz, ofst);
        if (ret < 0) {
    #if defined(WOLFSSL_NUCLEUS) && defined(DEBUG_WOLFSSH)
            if (ret == NUF_NOSPC) {
                WLOG(WS_LOG_SFTP, "Ran out of memory");
            }
    #endif
            WLOG(WS_LOG_SFTP, "Error writing to file");
            res  = err;
            type = WOLFSSH_FTP_FAILURE;
            ret  = WS_INVALID_STATE_E;
        }
        else {
            ret = WS_SUCCESS;
        }
    }

    if (wolfSSH_SFTP_CreateStatus(ssh, type, reqId, res, "English", NULL,
                &outSz) != WS_SIZE_ONLY) {
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