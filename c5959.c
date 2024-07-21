selaComputeCompositeParameters(const char  *fileout)
{
char    *str, *nameh1, *nameh2, *namev1, *namev2;
char     buf[L_BUF_SIZE];
l_int32  size, size1, size2, len;
SARRAY  *sa;
SELA    *selabasic, *selacomb;

    selabasic = selaAddBasic(NULL);
    selacomb = selaAddDwaCombs(NULL);
    sa = sarrayCreate(64);
    for (size = 2; size < 64; size++) {
        selectComposableSizes(size, &size1, &size2);
        nameh1 = selaGetBrickName(selabasic, size1, 1);
        namev1 = selaGetBrickName(selabasic, 1, size1);
        if (size2 > 1) {
            nameh2 = selaGetCombName(selacomb, size1 * size2, L_HORIZ);
            namev2 = selaGetCombName(selacomb, size1 * size2, L_VERT);
        } else {
            nameh2 = stringNew("");
            namev2 = stringNew("");
        }
        snprintf(buf, L_BUF_SIZE,
                 "      { %d, %d, %d, \"%s\", \"%s\", \"%s\", \"%s\" },",
                 size, size1, size2, nameh1, nameh2, namev1, namev2);
        sarrayAddString(sa, buf, L_COPY);
        LEPT_FREE(nameh1);
        LEPT_FREE(nameh2);
        LEPT_FREE(namev1);
        LEPT_FREE(namev2);
    }
    str = sarrayToString(sa, 1);
    len = strlen(str);
    l_binaryWrite(fileout, "w", str, len + 1);
    LEPT_FREE(str);
    sarrayDestroy(&sa);
    selaDestroy(&selabasic);
    selaDestroy(&selacomb);
    return;
}