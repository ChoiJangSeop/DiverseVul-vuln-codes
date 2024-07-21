pci_set_cfgdata8(struct pci_vdev *dev, int offset, uint8_t val)
{
	assert(offset <= PCI_REGMAX);
	*(uint8_t *)(dev->cfgdata + offset) = val;
}