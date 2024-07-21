xmlStrVPrintf(xmlChar *buf, int len, const xmlChar *msg, va_list ap) {
    int ret;

    if((buf == NULL) || (msg == NULL)) {
        return(-1);
    }

    ret = vsnprintf((char *) buf, len, (const char *) msg, ap);
    buf[len - 1] = 0; /* be safe ! */

    return(ret);
}