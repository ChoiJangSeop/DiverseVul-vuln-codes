pci_set_cfgdata32(struct pci_vdev *dev, int offset, uint32_t val)
{
	assert(offset <= (PCI_REGMAX - 3) && (offset & 3) == 0);
	*(uint32_t *)(dev->cfgdata + offset) = val;
}