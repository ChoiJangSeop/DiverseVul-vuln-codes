pci_get_cfgdata16(struct pci_vdev *dev, int offset)
{
	assert(offset <= (PCI_REGMAX - 1) && (offset & 1) == 0);
	return (*(uint16_t *)(dev->cfgdata + offset));
}