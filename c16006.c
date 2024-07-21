pci_lintr_route(struct pci_vdev *dev)
{
	struct businfo *bi;
	struct intxinfo *ii;

	if (dev->lintr.pin == 0)
		return;

	bi = pci_businfo[dev->bus];
	assert(bi != NULL);
	ii = &bi->slotinfo[dev->slot].si_intpins[dev->lintr.pin - 1];

	/*
	 * Attempt to allocate an I/O APIC pin for this intpin if one
	 * is not yet assigned.
	 */
	if (ii->ii_ioapic_irq == 0)
		ii->ii_ioapic_irq = ioapic_pci_alloc_irq(dev);
	assert(ii->ii_ioapic_irq > 0);

	/*
	 * Attempt to allocate a PIRQ pin for this intpin if one is
	 * not yet assigned.
	 */
	if (ii->ii_pirq_pin == 0)
		ii->ii_pirq_pin = pirq_alloc_pin(dev);
	assert(ii->ii_pirq_pin > 0);

	dev->lintr.ioapic_irq = ii->ii_ioapic_irq;
	dev->lintr.pirq_pin = ii->ii_pirq_pin;
	pci_set_cfgdata8(dev, PCIR_INTLINE, pirq_irq(ii->ii_pirq_pin));
}