static int tftp_session_find(Slirp *slirp, struct sockaddr_storage *srcsas,
                             struct tftp_t *tp)
{
    struct tftp_session *spt;
    int k;

    for (k = 0; k < TFTP_SESSIONS_MAX; k++) {
        spt = &slirp->tftp_sessions[k];

        if (tftp_session_in_use(spt)) {
            if (sockaddr_equal(&spt->client_addr, srcsas)) {
                if (spt->client_port == tp->udp.uh_sport) {
                    return k;
                }
            }
        }
    }

    return -1;
}