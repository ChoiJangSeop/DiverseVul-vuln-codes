static void tulip_xmit_list_update(TULIPState *s)
{
    struct tulip_descriptor desc;

    if (tulip_ts(s) != CSR5_TS_SUSPENDED) {
        return;
    }

    for (;;) {
        tulip_desc_read(s, s->current_tx_desc, &desc);
        tulip_dump_tx_descriptor(s, &desc);

        if (!(desc.status & TDES0_OWN)) {
            tulip_update_ts(s, CSR5_TS_SUSPENDED);
            s->csr[5] |= CSR5_TU;
            tulip_update_int(s);
            return;
        }

        if (desc.control & TDES1_SET) {
            tulip_setup_frame(s, &desc);
        } else {
            if (desc.control & TDES1_FS) {
                s->tx_frame_len = 0;
            }

            tulip_copy_tx_buffers(s, &desc);

            if (desc.control & TDES1_LS) {
                tulip_tx(s, &desc);
            }
        }
        tulip_desc_write(s, s->current_tx_desc, &desc);
        tulip_next_tx_descriptor(s, &desc);
    }
}