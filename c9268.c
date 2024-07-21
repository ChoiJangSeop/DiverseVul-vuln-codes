bool net_tx_pkt_add_raw_fragment(struct NetTxPkt *pkt, hwaddr pa,
    size_t len)
{
    hwaddr mapped_len = 0;
    struct iovec *ventry;
    assert(pkt);
    assert(pkt->max_raw_frags > pkt->raw_frags);

    if (!len) {
        return true;
     }

    ventry = &pkt->raw[pkt->raw_frags];
    mapped_len = len;

    ventry->iov_base = pci_dma_map(pkt->pci_dev, pa,
                                   &mapped_len, DMA_DIRECTION_TO_DEVICE);

    if ((ventry->iov_base != NULL) && (len == mapped_len)) {
        ventry->iov_len = mapped_len;
        pkt->raw_frags++;
        return true;
    } else {
        return false;
    }
}