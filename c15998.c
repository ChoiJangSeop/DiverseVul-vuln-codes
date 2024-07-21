pci_emul_cmdsts_write(struct pci_vdev *dev, int coff, uint32_t new, int bytes)
{
	int i, rshift;
	uint32_t cmd, cmd2, changed, old, readonly;

	cmd = pci_get_cfgdata16(dev, PCIR_COMMAND);	/* stash old value */

	/*
	 * From PCI Local Bus Specification 3.0 sections 6.2.2 and 6.2.3.
	 *
	 * XXX Bits 8, 11, 12, 13, 14 and 15 in the status register are
	 * 'write 1 to clear'. However these bits are not set to '1' by
	 * any device emulation so it is simpler to treat them as readonly.
	 */
	rshift = (coff & 0x3) * 8;
	readonly = 0xFFFFF880 >> rshift;

	old = CFGREAD(dev, coff, bytes);
	new &= ~readonly;
	new |= (old & readonly);
	CFGWRITE(dev, coff, new, bytes);		/* update config */

	cmd2 = pci_get_cfgdata16(dev, PCIR_COMMAND);	/* get updated value */
	changed = cmd ^ cmd2;

	/*
	 * If the MMIO or I/O address space decoding has changed then
	 * register/unregister all BARs that decode that address space.
	 */
	for (i = 0; i <= PCI_BARMAX; i++) {
		switch (dev->bar[i].type) {
		case PCIBAR_NONE:
		case PCIBAR_MEMHI64:
			break;
		case PCIBAR_IO:
		/* I/O address space decoding changed? */
			if (changed & PCIM_CMD_PORTEN) {
				if (porten(dev))
					register_bar(dev, i);
				else
					unregister_bar(dev, i);
			}
			break;
		case PCIBAR_MEM32:
		case PCIBAR_MEM64:
		/* MMIO address space decoding changed? */
			if (changed & PCIM_CMD_MEMEN) {
				if (memen(dev))
					register_bar(dev, i);
				else
					unregister_bar(dev, i);
			}
			break;
		default:
			assert(0);
		}
	}

	/*
	 * If INTx has been unmasked and is pending, assert the
	 * interrupt.
	 */
	pci_lintr_update(dev);
}