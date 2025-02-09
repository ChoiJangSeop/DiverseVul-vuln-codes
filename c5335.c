static QIOChannel *nbd_negotiate_handle_starttls(NBDClient *client,
                                                 uint32_t length)
{
    QIOChannel *ioc;
    QIOChannelTLS *tioc;
    struct NBDTLSHandshakeData data = { 0 };

    TRACE("Setting up TLS");
    ioc = client->ioc;
    if (length) {
        if (nbd_negotiate_drop_sync(ioc, length) < 0) {
            return NULL;
        }
        nbd_negotiate_send_rep_err(ioc, NBD_REP_ERR_INVALID, NBD_OPT_STARTTLS,
                                   "OPT_STARTTLS should not have length");
        return NULL;
    }

    if (nbd_negotiate_send_rep(client->ioc, NBD_REP_ACK,
                               NBD_OPT_STARTTLS) < 0) {
        return NULL;
    }

    tioc = qio_channel_tls_new_server(ioc,
                                      client->tlscreds,
                                      client->tlsaclname,
                                      NULL);
    if (!tioc) {
        return NULL;
    }

    qio_channel_set_name(QIO_CHANNEL(tioc), "nbd-server-tls");
    TRACE("Starting TLS handshake");
    data.loop = g_main_loop_new(g_main_context_default(), FALSE);
    qio_channel_tls_handshake(tioc,
                              nbd_tls_handshake,
                              &data,
                              NULL);

    if (!data.complete) {
        g_main_loop_run(data.loop);
    }
    g_main_loop_unref(data.loop);
    if (data.error) {
        object_unref(OBJECT(tioc));
        error_free(data.error);
        return NULL;
    }

    return QIO_CHANNEL(tioc);
}