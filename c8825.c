static void tulip_tx(TULIPState *s, struct tulip_descriptor *desc)
{
    if (s->tx_frame_len) {
        if ((s->csr[6] >> CSR6_OM_SHIFT) & CSR6_OM_MASK) {
            /* Internal or external Loopback */
            tulip_receive(s, s->tx_frame, s->tx_frame_len);
        } else {
            qemu_send_packet(qemu_get_queue(s->nic),
                s->tx_frame, s->tx_frame_len);
        }
    }

    if (desc->control & TDES1_IC) {
        s->csr[5] |= CSR5_TI;
        tulip_update_int(s);
    }
}