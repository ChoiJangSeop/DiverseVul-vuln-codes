sds *ldbReplParseCommand(int *argcp) {
    sds *argv = NULL;
    int argc = 0;
    if (sdslen(ldb.cbuf) == 0) return NULL;

    /* Working on a copy is simpler in this case. We can modify it freely
     * for the sake of simpler parsing. */
    sds copy = sdsdup(ldb.cbuf);
    char *p = copy;

    /* This Redis protocol parser is a joke... just the simplest thing that
     * works in this context. It is also very forgiving regarding broken
     * protocol. */

    /* Seek and parse *<count>\r\n. */
    p = strchr(p,'*'); if (!p) goto protoerr;
    char *plen = p+1; /* Multi bulk len pointer. */
    p = strstr(p,"\r\n"); if (!p) goto protoerr;
    *p = '\0'; p += 2;
    *argcp = atoi(plen);
    if (*argcp <= 0 || *argcp > 1024) goto protoerr;

    /* Parse each argument. */
    argv = zmalloc(sizeof(sds)*(*argcp));
    argc = 0;
    while(argc < *argcp) {
        if (*p != '$') goto protoerr;
        plen = p+1; /* Bulk string len pointer. */
        p = strstr(p,"\r\n"); if (!p) goto protoerr;
        *p = '\0'; p += 2;
        int slen = atoi(plen); /* Length of this arg. */
        if (slen <= 0 || slen > 1024) goto protoerr;
        argv[argc++] = sdsnewlen(p,slen);
        p += slen; /* Skip the already parsed argument. */
        if (p[0] != '\r' || p[1] != '\n') goto protoerr;
        p += 2; /* Skip \r\n. */
    }
    sdsfree(copy);
    return argv;

protoerr:
    sdsfreesplitres(argv,argc);
    sdsfree(copy);
    return NULL;
}