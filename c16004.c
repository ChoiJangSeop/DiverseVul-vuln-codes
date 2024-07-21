pci_cfgrw(struct vmctx *ctx, int vcpu, int in, int bus, int slot, int func,
	  int coff, int bytes, uint32_t *eax)
{
	struct businfo *bi;
	struct slotinfo *si;
	struct pci_vdev *dev;
	struct pci_vdev_ops *ops;
	int idx, needcfg;
	uint64_t addr, bar, mask;
	bool decode, ignore_reg_unreg = false;

	bi = pci_businfo[bus];
	if (bi != NULL) {
		si = &bi->slotinfo[slot];
		dev = si->si_funcs[func].fi_devi;
	} else
		dev = NULL;

	/*
	 * Just return if there is no device at this slot:func or if the
	 * the guest is doing an un-aligned access.
	 */
	if (dev == NULL || (bytes != 1 && bytes != 2 && bytes != 4) ||
	    (coff & (bytes - 1)) != 0) {
		if (in)
			*eax = 0xffffffff;
		return;
	}

	ops = dev->dev_ops;

	/*
	 * For non-passthru device, extended config space is NOT supported.
	 * Ignore all writes beyond the standard config space and return all
	 * ones on reads.
	 *
	 * For passthru device, extended config space is supported.
	 * Access to extended config space is implemented via libpciaccess.
	 */
	if (strcmp("passthru", ops->class_name)) {
		if (coff >= PCI_REGMAX + 1) {
			if (in) {
				*eax = 0xffffffff;
				/*
				 * Extended capabilities begin at offset 256 in
				 * config space.
				 * Absence of extended capabilities is signaled
				 * with all 0s in the extended capability header
				 * at offset 256.
				 */
				if (coff <= PCI_REGMAX + 4)
					*eax = 0x00000000;
			}
			return;
		}
	}

	/*
	 * Config read
	 */
	if (in) {
		/* Let the device emulation override the default handler */
		if (ops->vdev_cfgread != NULL) {
			needcfg = ops->vdev_cfgread(ctx, vcpu, dev, coff, bytes,
			    eax);
		} else {
			needcfg = 1;
		}

		if (needcfg)
			*eax = CFGREAD(dev, coff, bytes);

		pci_emul_hdrtype_fixup(bus, slot, coff, bytes, eax);
	} else {
		/* Let the device emulation override the default handler */
		if (ops->vdev_cfgwrite != NULL &&
		    (*ops->vdev_cfgwrite)(ctx, vcpu, dev,
					  coff, bytes, *eax) == 0)
			return;

		/*
		 * Special handling for write to BAR registers
		 */
		if (coff >= PCIR_BAR(0) && coff < PCIR_BAR(PCI_BARMAX + 1)) {
			/*
			 * Ignore writes to BAR registers that are not
			 * 4-byte aligned.
			 */
			if (bytes != 4 || (coff & 0x3) != 0)
				return;
			idx = (coff - PCIR_BAR(0)) / 4;
			mask = ~(dev->bar[idx].size - 1);

			if (dev->bar[idx].type == PCIBAR_IO)
				decode = porten(dev);
			else
				decode = memen(dev);

			/* Some driver does not disable the decode of BAR
			 * register via the command register before sizing a
			 * BAR. This will lead to a overlay of the BAR
			 * addresses when trying to register the intermediate
			 * BAR address via register_bar. A stateful variable
			 * sizing is used to keep track of such kind of BAR
			 * address changes and workaroud this violation.
			 */
			if (decode) {
				if (!dev->bar[idx].sizing && (*eax == ~0U)) {
					dev->bar[idx].sizing = true;
					ignore_reg_unreg = true;
				} else if (dev->bar[idx].sizing && (*eax != ~0U)) {
					dev->bar[idx].sizing = false;
					ignore_reg_unreg = true;
				}
			}

			switch (dev->bar[idx].type) {
			case PCIBAR_NONE:
				dev->bar[idx].addr = bar = 0;
				break;
			case PCIBAR_IO:
				addr = *eax & mask;
				addr &= 0xffff;
				bar = addr | PCIM_BAR_IO_SPACE;
				/*
				 * Register the new BAR value for interception
				 */
				if (addr != dev->bar[idx].addr) {
					update_bar_address(ctx, dev, addr, idx,
							   PCIBAR_IO,
							   ignore_reg_unreg);
				}
				break;
			case PCIBAR_MEM32:
				addr = bar = *eax & mask;
				bar |= PCIM_BAR_MEM_SPACE | PCIM_BAR_MEM_32;
				if (addr != dev->bar[idx].addr) {
					update_bar_address(ctx, dev, addr, idx,
							   PCIBAR_MEM32,
							   ignore_reg_unreg);
				}
				break;
			case PCIBAR_MEM64:
				addr = bar = *eax & mask;
				bar |= PCIM_BAR_MEM_SPACE | PCIM_BAR_MEM_64 |
				       PCIM_BAR_MEM_PREFETCH;
				if (addr != (uint32_t)dev->bar[idx].addr) {
					update_bar_address(ctx, dev, addr, idx,
							   PCIBAR_MEM64,
							   ignore_reg_unreg);
				}
				break;
			case PCIBAR_MEMHI64:
				assert(idx >= 1);
				mask = ~(dev->bar[idx - 1].size - 1);
				addr = ((uint64_t)*eax << 32) & mask;
				bar = addr >> 32;
				if (bar != dev->bar[idx - 1].addr >> 32) {
					update_bar_address(ctx, dev, addr, idx - 1,
							   PCIBAR_MEMHI64,
							   ignore_reg_unreg);
				}
				break;
			default:
				assert(0);
			}
			pci_set_cfgdata32(dev, coff, bar);

		} else if (coff == PCIR_BIOS) {
			/* ignore ROM BAR length request */
		} else if (pci_emul_iscap(dev, coff)) {
			pci_emul_capwrite(dev, coff, bytes, *eax);
		} else if (coff >= PCIR_COMMAND && coff < PCIR_REVID) {
			pci_emul_cmdsts_write(dev, coff, *eax, bytes);
		} else {
			CFGWRITE(dev, coff, *eax, bytes);
		}
	}
}