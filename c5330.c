static int nbd_negotiate_handle_list(NBDClient *client, uint32_t length)
{
    NBDExport *exp;

    if (length) {
        if (nbd_negotiate_drop_sync(client->ioc, length) < 0) {
            return -EIO;
        }
        return nbd_negotiate_send_rep_err(client->ioc,
                                          NBD_REP_ERR_INVALID, NBD_OPT_LIST,
                                          "OPT_LIST should not have length");
    }

    /* For each export, send a NBD_REP_SERVER reply. */
    QTAILQ_FOREACH(exp, &exports, next) {
        if (nbd_negotiate_send_rep_list(client->ioc, exp)) {
            return -EINVAL;
        }
    }
    /* Finish with a NBD_REP_ACK. */
    return nbd_negotiate_send_rep(client->ioc, NBD_REP_ACK, NBD_OPT_LIST);
}