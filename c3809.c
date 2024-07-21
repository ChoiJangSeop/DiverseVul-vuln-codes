xmlStrPrintf(xmlChar *buf, int len, const xmlChar *msg, ...) {
    va_list args;
    int ret;

    if((buf == NULL) || (msg == NULL)) {
        return(-1);
    }

    va_start(args, msg);
    ret = vsnprintf((char *) buf, len, (const char *) msg, args);
    va_end(args);
    buf[len - 1] = 0; /* be safe ! */

    return(ret);
}