pci_emul_dinit(struct vmctx *ctx, struct pci_vdev *dev, char *opts)
{
	int error;
	struct pci_emul_dummy *dummy;

	dummy = calloc(1, sizeof(struct pci_emul_dummy));

	dev->arg = dummy;

	pci_set_cfgdata16(dev, PCIR_DEVICE, 0x0001);
	pci_set_cfgdata16(dev, PCIR_VENDOR, 0x10DD);
	pci_set_cfgdata8(dev, PCIR_CLASS, 0x02);

	error = pci_emul_add_msicap(dev, PCI_EMUL_MSI_MSGS);
	assert(error == 0);

	error = pci_emul_alloc_bar(dev, 0, PCIBAR_IO, DIOSZ);
	assert(error == 0);

	error = pci_emul_alloc_bar(dev, 1, PCIBAR_MEM32, DMEMSZ);
	assert(error == 0);

	error = pci_emul_alloc_bar(dev, 2, PCIBAR_MEM32, DMEMSZ);
	assert(error == 0);

	return 0;
}