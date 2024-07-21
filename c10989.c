libvirt_vmessage(xentoollog_logger *logger_in,
                 xentoollog_level level,
                 int errnoval,
                 const char *context,
                 const char *format,
                 va_list args)
{
    xentoollog_logger_libvirt *lg = (xentoollog_logger_libvirt *)logger_in;
    FILE *logFile = lg->defaultLogFile;
    char timestamp[VIR_TIME_STRING_BUFLEN];
    g_autofree char *message = NULL;
    char *start, *end;

    VIR_DEBUG("libvirt_vmessage: context='%s' format='%s'", context, format);

    if (level < lg->minLevel)
        return;

    message = g_strdup_vprintf(format, args);

    /* Should we print to a domain-specific log file? */
    if ((start = strstr(message, ": Domain ")) &&
        (end = strstr(start + 9, ":"))) {
        FILE *domainLogFile;

        VIR_DEBUG("Found domain log message");

        start = start + 9;
        *end = '\0';

        domainLogFile = virHashLookup(lg->files, start);
        if (domainLogFile)
            logFile = domainLogFile;

        *end = ':';
    }

    /* Do the actual print to the log file */
    if (virTimeStringNowRaw(timestamp) < 0)
        timestamp[0] = '\0';

    fprintf(logFile, "%s: ", timestamp);
    if (context)
        fprintf(logFile, "%s: ", context);

    fprintf(logFile, "%s", message);

    if (errnoval >= 0)
        fprintf(logFile, ": %s", g_strerror(errnoval));

    fputc('\n', logFile);
    fflush(logFile);
}