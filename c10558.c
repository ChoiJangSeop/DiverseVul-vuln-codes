static void tftp_send_error(struct tftp_session *spt, uint16_t errorcode,
                            const char *msg, struct tftp_t *recv_tp)
{
    struct mbuf *m;
    struct tftp_t *tp;

    DEBUG_TFTP("tftp error msg: %s", msg);

    m = m_get(spt->slirp);

    if (!m) {
        goto out;
    }

    tp = tftp_prep_mbuf_data(spt, m);

    tp->tp_op = htons(TFTP_ERROR);
    tp->x.tp_error.tp_error_code = htons(errorcode);
    slirp_pstrcpy((char *)tp->x.tp_error.tp_msg, sizeof(tp->x.tp_error.tp_msg),
                  msg);

    m->m_len = sizeof(struct tftp_t) - (TFTP_BLOCKSIZE_MAX + 2) + 3 +
               strlen(msg) - sizeof(struct udphdr);
    tftp_udp_output(spt, m, recv_tp);

out:
    tftp_session_terminate(spt);
}