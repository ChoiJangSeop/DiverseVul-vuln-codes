int status(const char *fmt, ...) {
    static char buf[1024];
    va_list args;

    if(!show_status) return(0);
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    return(message(LOG_DEBUG, buf));
}