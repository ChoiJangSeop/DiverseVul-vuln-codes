pci_lintr_release(struct pci_vdev *dev)
{
	struct businfo *bi;
	struct slotinfo *si;
	int pin;

	bi = pci_businfo[dev->bus];
	assert(bi != NULL);

	si = &bi->slotinfo[dev->slot];

	for (pin = 1; pin < 4; pin++) {
		si->si_intpins[pin].ii_count = 0;
		si->si_intpins[pin].ii_pirq_pin = 0;
		si->si_intpins[pin].ii_ioapic_irq = 0;
	}
}