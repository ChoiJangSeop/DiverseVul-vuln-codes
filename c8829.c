static void tulip_copy_rx_bytes(TULIPState *s, struct tulip_descriptor *desc)
{
    int len1 = (desc->control >> RDES1_BUF1_SIZE_SHIFT) & RDES1_BUF1_SIZE_MASK;
    int len2 = (desc->control >> RDES1_BUF2_SIZE_SHIFT) & RDES1_BUF2_SIZE_MASK;
    int len;

    if (s->rx_frame_len && len1) {
        if (s->rx_frame_len > len1) {
            len = len1;
        } else {
            len = s->rx_frame_len;
        }
        pci_dma_write(&s->dev, desc->buf_addr1, s->rx_frame +
            (s->rx_frame_size - s->rx_frame_len), len);
        s->rx_frame_len -= len;
    }

    if (s->rx_frame_len && len2) {
        if (s->rx_frame_len > len2) {
            len = len2;
        } else {
            len = s->rx_frame_len;
        }
        pci_dma_write(&s->dev, desc->buf_addr2, s->rx_frame +
            (s->rx_frame_size - s->rx_frame_len), len);
        s->rx_frame_len -= len;
    }
}