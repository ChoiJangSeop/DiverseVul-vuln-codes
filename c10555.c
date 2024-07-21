static void tftp_send_next_block(struct tftp_session *spt,
                                 struct tftp_t *recv_tp)
{
    struct mbuf *m;
    struct tftp_t *tp;
    int nobytes;

    m = m_get(spt->slirp);

    if (!m) {
        return;
    }

    tp = tftp_prep_mbuf_data(spt, m);

    tp->tp_op = htons(TFTP_DATA);
    tp->x.tp_data.tp_block_nr = htons((spt->block_nr + 1) & 0xffff);

    nobytes = tftp_read_data(spt, spt->block_nr, tp->x.tp_data.tp_buf,
                             spt->block_size);

    if (nobytes < 0) {
        m_free(m);

        /* send "file not found" error back */

        tftp_send_error(spt, 1, "File not found", tp);

        return;
    }

    m->m_len = sizeof(struct tftp_t) - (TFTP_BLOCKSIZE_MAX - nobytes) -
               sizeof(struct udphdr);
    tftp_udp_output(spt, m, recv_tp);

    if (nobytes == spt->block_size) {
        tftp_session_update(spt);
    } else {
        tftp_session_terminate(spt);
    }

    spt->block_nr++;
}