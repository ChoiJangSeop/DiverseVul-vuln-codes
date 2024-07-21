static int tftp_send_oack(struct tftp_session *spt, const char *keys[],
                          uint32_t values[], int nb, struct tftp_t *recv_tp)
{
    struct mbuf *m;
    struct tftp_t *tp;
    int i, n = 0;

    m = m_get(spt->slirp);

    if (!m)
        return -1;

    tp = tftp_prep_mbuf_data(spt, m);

    tp->tp_op = htons(TFTP_OACK);
    for (i = 0; i < nb; i++) {
        n += slirp_fmt0(tp->x.tp_buf + n, sizeof(tp->x.tp_buf) - n, "%s", keys[i]);
        n += slirp_fmt0(tp->x.tp_buf + n, sizeof(tp->x.tp_buf) - n, "%u", values[i]);
    }

    m->m_len = G_SIZEOF_MEMBER(struct tftp_t, tp_op) + n;
    tftp_udp_output(spt, m, recv_tp);

    return 0;
}