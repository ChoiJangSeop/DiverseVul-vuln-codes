pci_get_cfgdata32(struct pci_vdev *dev, int offset)
{
	assert(offset <= (PCI_REGMAX - 3) && (offset & 3) == 0);
	return (*(uint32_t *)(dev->cfgdata + offset));
}