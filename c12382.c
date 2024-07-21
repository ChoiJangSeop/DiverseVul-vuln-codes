int wolfSSH_SFTP_RecvOpenDir(WOLFSSH* ssh, int reqId, byte* data, word32 maxSz)
#ifndef USE_WINDOWS_API
{
    WDIR  ctx;
    word32 sz;
    char*  dir;
    word32 idx = 0;
    int   ret = WS_SUCCESS;

    word32 outSz = sizeof(word32)*2 + WOLFSSH_SFTP_HEADER + UINT32_SZ;
    byte*  out = NULL;
    word32 id[2];
    byte idFlat[sizeof(word32) * 2];

    if (ssh == NULL) {
        return WS_BAD_ARGUMENT;
    }

    WLOG(WS_LOG_SFTP, "Receiving WOLFSSH_FTP_OPENDIR");

    if (sizeof(WFD) > WOLFSSH_MAX_HANDLE) {
        WLOG(WS_LOG_SFTP, "Handle size is too large");
        return WS_FATAL_ERROR;
    }

    /* get directory name */
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

    /* get directory handle */
    if (wolfSSH_CleanPath(ssh, dir) < 0) {
        WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);
        return WS_FATAL_ERROR;
    }

    if (WOPENDIR(ssh->fs, ssh->ctx->heap, &ctx, dir) != 0) {
        WLOG(WS_LOG_SFTP, "Error with opening directory");
        WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);
        if (wolfSSH_SFTP_CreateStatus(ssh, WOLFSSH_FTP_NOFILE, reqId,
                "Unable To Open Directory", "English", NULL, &outSz)
                != WS_SIZE_ONLY) {
                return WS_FATAL_ERROR;
        }
        ret = WS_BAD_FILE_E;
    }

    (void)reqId;

    /* add to directory list @TODO locking for thread safety */
    if (ret == WS_SUCCESS) {
        DIR_HANDLE* cur = (DIR_HANDLE*)WMALLOC(sizeof(DIR_HANDLE),
                ssh->ctx->heap, DYNTYPE_SFTP);
        if (cur == NULL) {
            WFREE(dir, ssh->ctx->heap, DYNTYPE_BUFFER);
            WCLOSEDIR(&ctx);
            return WS_MEMORY_E;
        }
#ifdef WOLFSSL_NUCLEUS
        WMEMCPY(&cur->dir, &ctx, sizeof(WDIR));
#else
        cur->dir  = ctx;
#endif
        cur->id[0] = id[0] = idCount[0];
        cur->id[1] = id[1] = idCount[1];
        c32toa(id[0], idFlat);
        c32toa(id[1], idFlat + UINT32_SZ);
        AddAssign64(idCount, 1);
        cur->isEof = 0;
        cur->next  = dirList;
        dirList    = cur;
        dirList->dirName = dir; /* take over ownership of buffer */
    }

    out = (byte*)WMALLOC(outSz, ssh->ctx->heap, DYNTYPE_BUFFER);
    if (out == NULL) {
        return WS_MEMORY_E;
    }

    if (ret == WS_SUCCESS) {
        SFTP_CreatePacket(ssh, WOLFSSH_FTP_HANDLE, out, outSz,
                idFlat, sizeof(idFlat));
    }
    else {
        if (wolfSSH_SFTP_CreateStatus(ssh, WOLFSSH_FTP_NOFILE, reqId,
                "Unable To Open Directory", "English", out, &outSz)
                != WS_SUCCESS) {
            WFREE(out, ssh->ctx->heap, DYNTYPE_BUFFER);
            return WS_FATAL_ERROR;
        }
    }

    /* set send out buffer, "out" is taken by ssh  */
    wolfSSH_SFTP_RecvSetSend(ssh, out, outSz);

    return ret;
}