static ssize_t tulip_receive(TULIPState *s, const uint8_t *buf, size_t size)
{
    struct tulip_descriptor desc;

    trace_tulip_receive(buf, size);

    if (size < 14 || size > 2048 || s->rx_frame_len || tulip_rx_stopped(s)) {
        return 0;
    }

    if (!tulip_filter_address(s, buf)) {
        return size;
    }

    do {
        tulip_desc_read(s, s->current_rx_desc, &desc);
        tulip_dump_rx_descriptor(s, &desc);

        if (!(desc.status & RDES0_OWN)) {
            s->csr[5] |= CSR5_RU;
            tulip_update_int(s);
            return s->rx_frame_size - s->rx_frame_len;
        }
        desc.status = 0;

        if (!s->rx_frame_len) {
            s->rx_frame_size = size + 4;
            s->rx_status = RDES0_LS |
                 ((s->rx_frame_size & RDES0_FL_MASK) << RDES0_FL_SHIFT);
            desc.status |= RDES0_FS;
            memcpy(s->rx_frame, buf, size);
            s->rx_frame_len = s->rx_frame_size;
        }

        tulip_copy_rx_bytes(s, &desc);

        if (!s->rx_frame_len) {
            desc.status |= s->rx_status;
            s->csr[5] |= CSR5_RI;
            tulip_update_int(s);
        }
        tulip_dump_rx_descriptor(s, &desc);
        tulip_desc_write(s, s->current_rx_desc, &desc);
        tulip_next_rx_descriptor(s, &desc);
    } while (s->rx_frame_len);
    return size;
}