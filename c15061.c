pci_emul_mem_handler(struct vmctx *ctx, int vcpu, int dir, uint64_t addr,
		     int size, uint64_t *val, void *arg1, long arg2)
{
	struct pci_vdev *pdi = arg1;
	struct pci_vdev_ops *ops = pdi->dev_ops;
	uint64_t offset;
	int bidx = (int) arg2;

	assert(bidx <= PCI_BARMAX);
	assert(pdi->bar[bidx].type == PCIBAR_MEM32 ||
	       pdi->bar[bidx].type == PCIBAR_MEM64);
	assert(addr >= pdi->bar[bidx].addr &&
	       addr + size <= pdi->bar[bidx].addr + pdi->bar[bidx].size);

	offset = addr - pdi->bar[bidx].addr;

	if (dir == MEM_F_WRITE) {
		if (size == 8) {
			(*ops->vdev_barwrite)(ctx, vcpu, pdi, bidx, offset,
					   4, *val & 0xffffffff);
			(*ops->vdev_barwrite)(ctx, vcpu, pdi, bidx, offset + 4,
					   4, *val >> 32);
		} else {
			(*ops->vdev_barwrite)(ctx, vcpu, pdi, bidx, offset,
					   size, bar_value(size, *val));
		}
	} else {
		if (size == 8) {
			uint64_t val_lo, val_hi;

			val_lo = (*ops->vdev_barread)(ctx, vcpu, pdi, bidx,
			                              offset, 4);
			val_lo = bar_value(4, val_lo);

			val_hi = (*ops->vdev_barread)(ctx, vcpu, pdi, bidx,
			                              offset + 4, 4);

			*val = val_lo | (val_hi << 32);
		} else {
			*val = (*ops->vdev_barread)(ctx, vcpu, pdi, bidx,
			                            offset, size);
			*val = bar_value(size, *val);
		}
	}

	return 0;
}