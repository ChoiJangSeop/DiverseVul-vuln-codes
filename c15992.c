pci_get_cfgdata8(struct pci_vdev *dev, int offset)
{
	assert(offset <= PCI_REGMAX);
	return (*(uint8_t *)(dev->cfgdata + offset));
}