int ioat3_dma_probe(struct ioatdma_device *device, int dca)
{
	struct pci_dev *pdev = device->pdev;
	int dca_en = system_has_dca_enabled(pdev);
	struct dma_device *dma;
	struct dma_chan *c;
	struct ioat_chan_common *chan;
	bool is_raid_device = false;
	int err;

	device->enumerate_channels = ioat2_enumerate_channels;
	device->reset_hw = ioat3_reset_hw;
	device->self_test = ioat3_dma_self_test;
	device->intr_quirk = ioat3_intr_quirk;
	dma = &device->common;
	dma->device_prep_dma_memcpy = ioat2_dma_prep_memcpy_lock;
	dma->device_issue_pending = ioat2_issue_pending;
	dma->device_alloc_chan_resources = ioat2_alloc_chan_resources;
	dma->device_free_chan_resources = ioat2_free_chan_resources;

	dma_cap_set(DMA_INTERRUPT, dma->cap_mask);
	dma->device_prep_dma_interrupt = ioat3_prep_interrupt_lock;

	device->cap = readl(device->reg_base + IOAT_DMA_CAP_OFFSET);

	if (is_xeon_cb32(pdev) || is_bwd_noraid(pdev))
		device->cap &= ~(IOAT_CAP_XOR | IOAT_CAP_PQ | IOAT_CAP_RAID16SS);

	/* dca is incompatible with raid operations */
	if (dca_en && (device->cap & (IOAT_CAP_XOR|IOAT_CAP_PQ)))
		device->cap &= ~(IOAT_CAP_XOR|IOAT_CAP_PQ);

	if (device->cap & IOAT_CAP_XOR) {
		is_raid_device = true;
		dma->max_xor = 8;

		dma_cap_set(DMA_XOR, dma->cap_mask);
		dma->device_prep_dma_xor = ioat3_prep_xor;

		dma_cap_set(DMA_XOR_VAL, dma->cap_mask);
		dma->device_prep_dma_xor_val = ioat3_prep_xor_val;
	}

	if (device->cap & IOAT_CAP_PQ) {
		is_raid_device = true;

		dma->device_prep_dma_pq = ioat3_prep_pq;
		dma->device_prep_dma_pq_val = ioat3_prep_pq_val;
		dma_cap_set(DMA_PQ, dma->cap_mask);
		dma_cap_set(DMA_PQ_VAL, dma->cap_mask);

		if (device->cap & IOAT_CAP_RAID16SS) {
			dma_set_maxpq(dma, 16, 0);
		} else {
			dma_set_maxpq(dma, 8, 0);
		}

		if (!(device->cap & IOAT_CAP_XOR)) {
			dma->device_prep_dma_xor = ioat3_prep_pqxor;
			dma->device_prep_dma_xor_val = ioat3_prep_pqxor_val;
			dma_cap_set(DMA_XOR, dma->cap_mask);
			dma_cap_set(DMA_XOR_VAL, dma->cap_mask);

			if (device->cap & IOAT_CAP_RAID16SS) {
				dma->max_xor = 16;
			} else {
				dma->max_xor = 8;
			}
		}
	}

	dma->device_tx_status = ioat3_tx_status;
	device->cleanup_fn = ioat3_cleanup_event;
	device->timer_fn = ioat3_timer_event;

	/* starting with CB3.3 super extended descriptors are supported */
	if (device->cap & IOAT_CAP_RAID16SS) {
		char pool_name[14];
		int i;

		for (i = 0; i < MAX_SED_POOLS; i++) {
			snprintf(pool_name, 14, "ioat_hw%d_sed", i);

			/* allocate SED DMA pool */
			device->sed_hw_pool[i] = dmam_pool_create(pool_name,
					&pdev->dev,
					SED_SIZE * (i + 1), 64, 0);
			if (!device->sed_hw_pool[i])
				return -ENOMEM;

		}
	}

	err = ioat_probe(device);
	if (err)
		return err;
	ioat_set_tcp_copy_break(262144);

	list_for_each_entry(c, &dma->channels, device_node) {
		chan = to_chan_common(c);
		writel(IOAT_DMA_DCA_ANY_CPU,
		       chan->reg_base + IOAT_DCACTRL_OFFSET);
	}

	err = ioat_register(device);
	if (err)
		return err;

	ioat_kobject_add(device, &ioat2_ktype);

	if (dca)
		device->dca = ioat3_dca_init(pdev, device->reg_base);

	return 0;
}