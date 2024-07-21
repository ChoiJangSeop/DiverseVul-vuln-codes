pci_bus_configured(int bus)
{
	assert(bus >= 0 && bus < MAXBUSES);
	return (pci_businfo[bus] != NULL);
}