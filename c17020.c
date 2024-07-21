int ssh_scp_write(ssh_scp scp, const void *buffer, size_t len)
{
    int w;
    int rc;
    uint8_t code;

    if (scp == NULL) {
        return SSH_ERROR;
    }

    if (scp->state != SSH_SCP_WRITE_WRITING) {
        ssh_set_error(scp->session, SSH_FATAL,
                      "ssh_scp_write called under invalid state");
        return SSH_ERROR;
    }

    if (scp->processed + len > scp->filelen) {
        len = (size_t) (scp->filelen - scp->processed);
    }

    /* hack to avoid waiting for window change */
    rc = ssh_channel_poll(scp->channel, 0);
    if (rc == SSH_ERROR) {
        scp->state = SSH_SCP_ERROR;
        return SSH_ERROR;
    }

    w = ssh_channel_write(scp->channel, buffer, len);
    if (w != SSH_ERROR) {
        scp->processed += w;
    } else {
        scp->state = SSH_SCP_ERROR;
        //return = channel_get_exit_status(scp->channel);
        return SSH_ERROR;
    }

    /* Far end sometimes send a status message, which we need to read
     * and handle */
    rc = ssh_channel_poll(scp->channel, 0);
    if (rc > 0) {
        rc = ssh_channel_read(scp->channel, &code, 1, 0);
        if (rc == SSH_ERROR) {
            return SSH_ERROR;
        }

        if (code == 1 || code == 2) {
            ssh_set_error(scp->session, SSH_REQUEST_DENIED,
                          "SCP: Error: status code %i received", code);
            return SSH_ERROR;
        }
    }

    /* Check if we arrived at end of file */
    if (scp->processed == scp->filelen) {
        code = 0;
        w = ssh_channel_write(scp->channel, &code, 1);
        if (w == SSH_ERROR) {
            scp->state = SSH_SCP_ERROR;
            return SSH_ERROR;
        }

        scp->processed = scp->filelen = 0;
        scp->state = SSH_SCP_WRITE_INITED;
    }

    return SSH_OK;
}