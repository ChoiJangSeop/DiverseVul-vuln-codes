fmtstr(char **sbuffer,
       char **buffer,
       size_t *currlen,
       size_t *maxlen, const char *value, int flags, int min, int max)
{
    int padlen, strln;
    int cnt = 0;

    if (value == 0)
        value = "<NULL>";
    for (strln = 0; value[strln]; ++strln) ;
    padlen = min - strln;
    if (padlen < 0)
        padlen = 0;
    if (flags & DP_F_MINUS)
        padlen = -padlen;

    while ((padlen > 0) && (cnt < max)) {
        doapr_outch(sbuffer, buffer, currlen, maxlen, ' ');
        --padlen;
        ++cnt;
    }
    while (*value && (cnt < max)) {
        doapr_outch(sbuffer, buffer, currlen, maxlen, *value++);
        ++cnt;
    }
    while ((padlen < 0) && (cnt < max)) {
        doapr_outch(sbuffer, buffer, currlen, maxlen, ' ');
        ++padlen;
        ++cnt;
    }
}