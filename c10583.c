static errno_t sssctl_backup(bool force)
{
    const char *files[] = {SSS_BACKUP_USER_OVERRIDES,
                           SSS_BACKUP_GROUP_OVERRIDES,
                           NULL};
    enum sssctl_prompt_result prompt;
    errno_t ret;

    ret = sssctl_create_backup_dir(SSS_BACKUP_DIR);
    if (ret != EOK) {
        ERROR("Unable to create backup directory [%d]: %s",
              ret, sss_strerror(ret));
        return ret;
    }

    if (sssctl_backup_exist(files) && !force) {
        prompt = sssctl_prompt(_("SSSD backup of local data already exists, "
                                 "override?"), SSSCTL_PROMPT_NO);
        switch (prompt) {
        case SSSCTL_PROMPT_YES:
            /* continue */
            break;
        case SSSCTL_PROMPT_NO:
            return EEXIST;
        case SSSCTL_PROMPT_ERROR:
            return EIO;
        }
    }

    ret = sssctl_run_command("sss_override user-export "
                             SSS_BACKUP_USER_OVERRIDES);
    if (ret != EOK) {
        ERROR("Unable to export user overrides\n");
        return ret;
    }

    ret = sssctl_run_command("sss_override group-export "
                             SSS_BACKUP_GROUP_OVERRIDES);
    if (ret != EOK) {
        ERROR("Unable to export group overrides\n");
        return ret;
    }

    return ret;
}