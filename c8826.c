static void tulip_copy_tx_buffers(TULIPState *s, struct tulip_descriptor *desc)
{
    int len1 = (desc->control >> TDES1_BUF1_SIZE_SHIFT) & TDES1_BUF1_SIZE_MASK;
    int len2 = (desc->control >> TDES1_BUF2_SIZE_SHIFT) & TDES1_BUF2_SIZE_MASK;

    if (len1) {
        pci_dma_read(&s->dev, desc->buf_addr1,
            s->tx_frame + s->tx_frame_len, len1);
        s->tx_frame_len += len1;
    }

    if (len2) {
        pci_dma_read(&s->dev, desc->buf_addr2,
            s->tx_frame + s->tx_frame_len, len2);
        s->tx_frame_len += len2;
    }
    desc->status = (len1 + len2) ? 0 : 0x7fffffff;
}