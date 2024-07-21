pci_set_cfgdata16(struct pci_vdev *dev, int offset, uint16_t val)
{
	assert(offset <= (PCI_REGMAX - 1) && (offset & 1) == 0);
	*(uint16_t *)(dev->cfgdata + offset) = val;
}