int ioat2_dma_probe(struct ioatdma_device *device, int dca)
{
	struct pci_dev *pdev = device->pdev;
	struct dma_device *dma;
	struct dma_chan *c;
	struct ioat_chan_common *chan;
	int err;

	device->enumerate_channels = ioat2_enumerate_channels;
	device->reset_hw = ioat2_reset_hw;
	device->cleanup_fn = ioat2_cleanup_event;
	device->timer_fn = ioat2_timer_event;
	device->self_test = ioat_dma_self_test;
	dma = &device->common;
	dma->device_prep_dma_memcpy = ioat2_dma_prep_memcpy_lock;
	dma->device_issue_pending = ioat2_issue_pending;
	dma->device_alloc_chan_resources = ioat2_alloc_chan_resources;
	dma->device_free_chan_resources = ioat2_free_chan_resources;
	dma->device_tx_status = ioat_dma_tx_status;

	err = ioat_probe(device);
	if (err)
		return err;
	ioat_set_tcp_copy_break(2048);

	list_for_each_entry(c, &dma->channels, device_node) {
		chan = to_chan_common(c);
		writel(IOAT_DCACTRL_CMPL_WRITE_ENABLE | IOAT_DMA_DCA_ANY_CPU,
		       chan->reg_base + IOAT_DCACTRL_OFFSET);
	}

	err = ioat_register(device);
	if (err)
		return err;

	ioat_kobject_add(device, &ioat2_ktype);

	if (dca)
		device->dca = ioat2_dca_init(pdev, device->reg_base);

	return err;
}