errno_t sssctl_logs_fetch(struct sss_cmdline *cmdline,
                          struct sss_tool_ctx *tool_ctx,
                          void *pvt)
{
    const char *file;
    const char *cmd;
    errno_t ret;

    /* Parse command line. */
    ret = sss_tool_popt_ex(cmdline, NULL, SSS_TOOL_OPT_OPTIONAL, NULL, NULL,
                           "FILE", "Output file", &file, NULL);
    if (ret != EOK) {
        DEBUG(SSSDBG_CRIT_FAILURE, "Unable to parse command arguments\n");
        return ret;
    }

    cmd = talloc_asprintf(tool_ctx, "tar -czf %s %s", file, LOG_FILES);
    if (cmd == NULL) {
        ERROR("Out of memory!");
    }

    PRINT("Archiving log files into %s...\n", file);
    ret = sssctl_run_command(cmd);
    if (ret != EOK) {
        ERROR("Unable to archive log files\n");
        return ret;
    }

    return EOK;
}