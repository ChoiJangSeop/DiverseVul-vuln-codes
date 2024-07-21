static void tftp_handle_ack(Slirp *slirp, struct sockaddr_storage *srcsas,
                            struct tftp_t *tp, int pktlen)
{
    int s;

    s = tftp_session_find(slirp, srcsas, tp);

    if (s < 0) {
        return;
    }

    tftp_send_next_block(&slirp->tftp_sessions[s], tp);
}