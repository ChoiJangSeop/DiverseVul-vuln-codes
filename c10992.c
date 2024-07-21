libxlLoggerOpenFile(libxlLogger *logger,
                    int id,
                    const char *name,
                    const char *domain_config)
{
    g_autofree char *path = NULL;
    FILE *logFile = NULL;
    g_autofree char *domidstr = NULL;

    path = g_strdup_printf("%s/%s.log", logger->logDir, name);
    domidstr = g_strdup_printf("%d", id);

    if (!(logFile = fopen(path, "a"))) {
        VIR_WARN("Failed to open log file %s: %s",
                 path, g_strerror(errno));
        return;
    }
    ignore_value(virHashAddEntry(logger->files, domidstr, logFile));

    /* domain_config is non NULL only when starting a new domain */
    if (domain_config) {
        fprintf(logFile, "Domain start: %s\n", domain_config);
        fflush(logFile);
    }
}