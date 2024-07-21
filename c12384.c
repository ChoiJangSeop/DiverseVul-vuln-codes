int wolfSSH_SFTP_RecvRead(WOLFSSH* ssh, int reqId, byte* data, word32 maxSz)
#ifndef USE_WINDOWS_API
{
    WFD    fd;
    word32 sz;
    int    ret;
    word32 idx  = 0;
    word32 ofst[2] = {0, 0};

    byte*  out;
    word32 outSz;

    char* res  = NULL;
    char err[] = "Read File Error";
    char eof[] = "Read EOF";
    byte type = WOLFSSH_FTP_FAILURE;

    if (ssh == NULL) {
        return WS_BAD_ARGUMENT;
    }

    WLOG(WS_LOG_SFTP, "Receiving WOLFSSH_FTP_READ");

    /* get file handle */
    ato32(data + idx, &sz); idx += UINT32_SZ;
    if (sz + idx > maxSz || sz > WOLFSSH_MAX_HANDLE) {
        return WS_BUFFER_E;
    }
    WMEMSET((byte*)&fd, 0, sizeof(WFD));
    WMEMCPY((byte*)&fd, data + idx, sz); idx += sz;

    /* get offset into file */
    ato32(data + idx, &ofst[1]); idx += UINT32_SZ;
    ato32(data + idx, &ofst[0]); idx += UINT32_SZ;

    /* get length to be read */
    ato32(data + idx, &sz);

    /* read from handle and send data back to client */
    out = (byte*)WMALLOC(sz + WOLFSSH_SFTP_HEADER + UINT32_SZ,
            ssh->ctx->heap, DYNTYPE_BUFFER);
    if (out == NULL) {
        return WS_MEMORY_E;
    }

    ret = WPREAD(fd, out + UINT32_SZ + WOLFSSH_SFTP_HEADER, sz, ofst);
    if (ret < 0 || (word32)ret > sz) {
        WLOG(WS_LOG_SFTP, "Error reading from file");
        res  = err;
        type = WOLFSSH_FTP_FAILURE;
        ret  = WS_BAD_FILE_E;
    }
    else {
        outSz = (word32)ret + WOLFSSH_SFTP_HEADER + UINT32_SZ;
    }

    /* eof */
    if (ret == 0) {
        WLOG(WS_LOG_SFTP, "Error reading from file, EOF");
        res = eof;
        type = WOLFSSH_FTP_EOF;
        ret = WS_SUCCESS; /* end of file is not fatal error */
    }

    if (res != NULL) {
        if (wolfSSH_SFTP_CreateStatus(ssh, type, reqId, res, "English", NULL,
                &outSz) != WS_SIZE_ONLY) {
            WFREE(out, ssh->ctx->heap, DYNTYPE_BUFFER);
            return WS_FATAL_ERROR;
        }
        if (outSz > sz) {
            /* need to increase buffer size for holding status packet */
            WFREE(out, ssh->ctx->heap, DYNTYPE_BUFFER);
            out = (byte*)WMALLOC(outSz, ssh->ctx->heap, DYNTYPE_BUFFER);
            if (out == NULL) {
                return WS_MEMORY_E;
            }
        }
        if (wolfSSH_SFTP_CreateStatus(ssh, type, reqId, res, "English", out,
                    &outSz) != WS_SUCCESS) {
            WFREE(out, ssh->ctx->heap, DYNTYPE_BUFFER);
            return WS_FATAL_ERROR;
        }
    }
    else {
        SFTP_CreatePacket(ssh, WOLFSSH_FTP_DATA, out, outSz, NULL, 0);
    }

    /* set send out buffer, "out" is taken by ssh  */
    wolfSSH_SFTP_RecvSetSend(ssh, out, outSz);
    return ret;
}