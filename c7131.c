int warn(const char *fmt, ...) {
    static char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    return(message(LOG_WARNING, buf));
}