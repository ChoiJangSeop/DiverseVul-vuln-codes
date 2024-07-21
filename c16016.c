pci_get_vdev_info(int slot)
{
	struct businfo *bi;
	struct slotinfo *si;
	struct pci_vdev *dev = NULL;

	bi = pci_businfo[0];
	assert(bi != NULL);

	si = &bi->slotinfo[slot];
	if (si != NULL)
		dev = si->si_funcs[0].fi_devi;
	else
		fprintf(stderr, "slot=%d is empty!\n", slot);

	return dev;
}