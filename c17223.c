quit(int status, const char *fmt,...)
{
    va_list argp;

    va_start(argp, fmt);
    fprintf(stderr, "%s: ", prg_name);
    vfprintf(stderr, fmt, argp);
    fprintf(stderr, "\n");
    va_end(argp);
    ExitProgram(status);
}