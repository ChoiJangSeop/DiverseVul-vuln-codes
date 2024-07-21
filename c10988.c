libxlLoggerNew(const char *logDir, virLogPriority minLevel)
{
    xentoollog_logger_libvirt logger;
    g_autofree char *path = NULL;

    switch (minLevel) {
    case VIR_LOG_DEBUG:
        logger.minLevel = XTL_DEBUG;
        break;
    case VIR_LOG_INFO:
        logger.minLevel = XTL_INFO;
        break;
    case VIR_LOG_WARN:
        logger.minLevel = XTL_WARN;
        break;
    case VIR_LOG_ERROR:
        logger.minLevel = XTL_ERROR;
        break;
    }
    logger.logDir = logDir;

    path = g_strdup_printf("%s/libxl-driver.log", logDir);

    if ((logger.defaultLogFile = fopen(path, "a")) == NULL)
        return NULL;

    logger.files = virHashNew(libxlLoggerFileFree);

    return XTL_NEW_LOGGER(libvirt, logger);
}