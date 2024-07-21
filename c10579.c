errno_t sssctl_logs_remove(struct sss_cmdline *cmdline,
                           struct sss_tool_ctx *tool_ctx,
                           void *pvt)
{
    struct sssctl_logs_opts opts = {0};
    errno_t ret;

    /* Parse command line. */
    struct poptOption options[] = {
        {"delete", 'd', POPT_ARG_NONE, &opts.delete, 0, _("Delete log files instead of truncating"), NULL },
        POPT_TABLEEND
    };

    ret = sss_tool_popt(cmdline, options, SSS_TOOL_OPT_OPTIONAL, NULL, NULL);
    if (ret != EOK) {
        DEBUG(SSSDBG_CRIT_FAILURE, "Unable to parse command arguments\n");
        return ret;
    }

    if (opts.delete) {
        PRINT("Deleting log files...\n");
        ret = sss_remove_subtree(LOG_PATH);
        if (ret != EOK) {
            ERROR("Unable to remove log files\n");
            return ret;
        }

        sss_signal(SIGHUP);
    } else {
        PRINT("Truncating log files...\n");
        ret = sssctl_run_command("truncate --no-create --size 0 " LOG_FILES);
        if (ret != EOK) {
            ERROR("Unable to truncate log files\n");
            return ret;
        }
    }

    return EOK;
}