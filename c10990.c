libxlLoggerCloseFile(libxlLogger *logger, int id)
{
    g_autofree char *domidstr = NULL;
    domidstr = g_strdup_printf("%d", id);

    ignore_value(virHashRemoveEntry(logger->files, domidstr));
}