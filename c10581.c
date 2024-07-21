errno_t sssctl_cache_expire(struct sss_cmdline *cmdline,
                            struct sss_tool_ctx *tool_ctx,
                            void *pvt)
{
    errno_t ret;
    char *cmd_args = NULL;
    const char *cachecmd = SSS_CACHE;
    char *cmd = NULL;
    int i;

    if (cmdline->argc == 0) {
        ret = sssctl_run_command(cachecmd);
        goto done;
    }

    cmd_args = talloc_strdup(tool_ctx, "");
    if (cmd_args == NULL) {
        ret = ENOMEM;
        goto done;
    }

    for (i = 0; i < cmdline->argc; i++) {
        cmd_args = talloc_strdup_append(cmd_args, cmdline->argv[i]);
        if (i != cmdline->argc - 1) {
            cmd_args = talloc_strdup_append(cmd_args, " ");
        }
    }

    cmd = talloc_asprintf(tool_ctx, "%s %s", cachecmd, cmd_args);
    if (cmd == NULL) {
        ret = ENOMEM;
        goto done;
    }

    ret = sssctl_run_command(cmd);

done:
    talloc_free(cmd_args);
    talloc_free(cmd);

    return ret;
}