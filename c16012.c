pci_emul_capwrite(struct pci_vdev *dev, int offset, int bytes, uint32_t val)
{
	int capid;
	uint8_t capoff, nextoff;

	/* Do not allow un-aligned writes */
	if ((offset & (bytes - 1)) != 0)
		return;

	/* Find the capability that we want to update */
	capoff = CAP_START_OFFSET;
	while (1) {
		nextoff = pci_get_cfgdata8(dev, capoff + 1);
		if (nextoff == 0)
			break;
		if (offset >= capoff && offset < nextoff)
			break;

		capoff = nextoff;
	}
	assert(offset >= capoff);

	/*
	 * Capability ID and Next Capability Pointer are readonly.
	 * However, some o/s's do 4-byte writes that include these.
	 * For this case, trim the write back to 2 bytes and adjust
	 * the data.
	 */
	if (offset == capoff || offset == capoff + 1) {
		if (offset == capoff && bytes == 4) {
			bytes = 2;
			offset += 2;
			val >>= 16;
		} else
			return;
	}

	capid = pci_get_cfgdata8(dev, capoff);
	switch (capid) {
	case PCIY_MSI:
		msicap_cfgwrite(dev, capoff, offset, bytes, val);
		break;
	case PCIY_MSIX:
		msixcap_cfgwrite(dev, capoff, offset, bytes, val);
		break;
	case PCIY_EXPRESS:
		pciecap_cfgwrite(dev, capoff, offset, bytes, val);
		break;
	default:
		CFGWRITE(dev, offset, val, bytes);
		break;
	}
}