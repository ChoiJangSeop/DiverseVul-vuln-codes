int panic(const char *fmt, ...) {
    static char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    message(LOG_ERR, buf);
    exit(1);
}