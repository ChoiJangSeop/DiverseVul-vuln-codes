errno_t sssctl_run_command(const char *command)
{
    int ret;

    DEBUG(SSSDBG_TRACE_FUNC, "Running %s\n", command);

    ret = system(command);
    if (ret == -1) {
        DEBUG(SSSDBG_CRIT_FAILURE, "Unable to execute %s\n", command);
        ERROR("Error while executing external command\n");
        return EFAULT;
    } else if (WEXITSTATUS(ret) != 0) {
        DEBUG(SSSDBG_CRIT_FAILURE, "Command %s failed with [%d]\n",
              command, WEXITSTATUS(ret));
        ERROR("Error while executing external command\n");
        return EIO;
    }

    return EOK;
}