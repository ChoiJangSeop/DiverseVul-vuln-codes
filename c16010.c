pci_lintr_request(struct pci_vdev *dev)
{
	struct businfo *bi;
	struct slotinfo *si;
	int bestpin, bestcount, pin;

	bi = pci_businfo[dev->bus];
	assert(bi != NULL);

	/*
	 * Just allocate a pin from our slot.  The pin will be
	 * assigned IRQs later when interrupts are routed.
	 */
	si = &bi->slotinfo[dev->slot];
	bestpin = 0;
	bestcount = si->si_intpins[0].ii_count;
	for (pin = 1; pin < 4; pin++) {
		if (si->si_intpins[pin].ii_count < bestcount) {
			bestpin = pin;
			bestcount = si->si_intpins[pin].ii_count;
		}
	}

	si->si_intpins[bestpin].ii_count++;
	dev->lintr.pin = bestpin + 1;
	pci_set_cfgdata8(dev, PCIR_INTPIN, bestpin + 1);
}