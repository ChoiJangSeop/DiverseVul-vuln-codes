void tftp_input(struct sockaddr_storage *srcsas, struct mbuf *m)
{
    struct tftp_t *tp = (struct tftp_t *)m->m_data;

    switch (ntohs(tp->tp_op)) {
    case TFTP_RRQ:
        tftp_handle_rrq(m->slirp, srcsas, tp, m->m_len);
        break;

    case TFTP_ACK:
        tftp_handle_ack(m->slirp, srcsas, tp, m->m_len);
        break;

    case TFTP_ERROR:
        tftp_handle_error(m->slirp, srcsas, tp, m->m_len);
        break;
    }
}