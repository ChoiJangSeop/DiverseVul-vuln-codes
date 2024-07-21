static void tulip_desc_read(TULIPState *s, hwaddr p,
        struct tulip_descriptor *desc)
{
    const MemTxAttrs attrs = MEMTXATTRS_UNSPECIFIED;

    if (s->csr[0] & CSR0_DBO) {
        ldl_be_pci_dma(&s->dev, p, &desc->status, attrs);
        ldl_be_pci_dma(&s->dev, p + 4, &desc->control, attrs);
        ldl_be_pci_dma(&s->dev, p + 8, &desc->buf_addr1, attrs);
        ldl_be_pci_dma(&s->dev, p + 12, &desc->buf_addr2, attrs);
    } else {
        ldl_le_pci_dma(&s->dev, p, &desc->status, attrs);
        ldl_le_pci_dma(&s->dev, p + 4, &desc->control, attrs);
        ldl_le_pci_dma(&s->dev, p + 8, &desc->buf_addr1, attrs);
        ldl_le_pci_dma(&s->dev, p + 12, &desc->buf_addr2, attrs);
    }
}