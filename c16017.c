pci_emul_add_msicap(struct pci_vdev *dev, int msgnum)
{
	struct msicap msicap;

	pci_populate_msicap(&msicap, msgnum, 0);

	return pci_emul_add_capability(dev, (u_char *)&msicap, sizeof(msicap));
}