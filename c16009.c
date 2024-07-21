update_bar_address(struct vmctx *ctx, struct pci_vdev *dev, uint64_t addr,
	int idx, int type, bool ignore_reg_unreg)
{
	bool decode = false;
	uint64_t orig_addr = dev->bar[idx].addr;

	if (!ignore_reg_unreg) {
		if (dev->bar[idx].type == PCIBAR_IO)
			decode = porten(dev);
		else
			decode = memen(dev);
	}

	if (decode)
		unregister_bar(dev, idx);

	switch (type) {
	case PCIBAR_IO:
	case PCIBAR_MEM32:
		dev->bar[idx].addr = addr;
		break;
	case PCIBAR_MEM64:
		dev->bar[idx].addr &= ~0xffffffffUL;
		dev->bar[idx].addr |= addr;
		break;
	case PCIBAR_MEMHI64:
		dev->bar[idx].addr &= 0xffffffff;
		dev->bar[idx].addr |= addr;
		break;
	default:
		assert(0);
	}

	if (decode)
		register_bar(dev, idx);

	/* update bar mapping */
	if (dev->dev_ops->vdev_update_bar_map && decode)
		dev->dev_ops->vdev_update_bar_map(ctx, dev, idx, orig_addr);
}