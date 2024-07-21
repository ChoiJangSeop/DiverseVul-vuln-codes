int ssh_scp_push_directory(ssh_scp scp, const char *dirname, int mode)
{
    char buffer[1024] = {0};
    int rc;
    uint8_t code;
    char *dir = NULL;
    char *perms = NULL;

    if (scp == NULL) {
        return SSH_ERROR;
    }

    if (scp->state != SSH_SCP_WRITE_INITED) {
        ssh_set_error(scp->session, SSH_FATAL,
                      "ssh_scp_push_directory called under invalid state");
        return SSH_ERROR;
    }

    dir = ssh_basename(dirname);
    perms = ssh_scp_string_mode(mode);
    snprintf(buffer, sizeof(buffer), "D%s 0 %s\n", perms, dir);
    SAFE_FREE(dir);
    SAFE_FREE(perms);

    rc = ssh_channel_write(scp->channel, buffer, strlen(buffer));
    if (rc == SSH_ERROR) {
        scp->state = SSH_SCP_ERROR;
        return SSH_ERROR;
    }

    rc = ssh_channel_read(scp->channel, &code, 1, 0);
    if (rc <= 0) {
        ssh_set_error(scp->session, SSH_FATAL,
                      "Error reading status code: %s",
                      ssh_get_error(scp->session));
        scp->state = SSH_SCP_ERROR;
        return SSH_ERROR;
    }

    if (code != 0) {
        ssh_set_error(scp->session, SSH_FATAL, "scp status code %ud not valid",
                      code);
        scp->state = SSH_SCP_ERROR;
        return SSH_ERROR;
    }

    return SSH_OK;
}