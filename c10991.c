libxlLoggerFree(libxlLogger *logger)
{
    xentoollog_logger *xtl_logger = (xentoollog_logger*)logger;
    if (logger->defaultLogFile)
        VIR_FORCE_FCLOSE(logger->defaultLogFile);
    g_clear_pointer(&logger->files, g_hash_table_unref);
    xtl_logger_destroy(xtl_logger);
}