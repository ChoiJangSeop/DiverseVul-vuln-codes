int ioat1_dma_probe(struct ioatdma_device *device, int dca)
{
	struct pci_dev *pdev = device->pdev;
	struct dma_device *dma;
	int err;

	device->intr_quirk = ioat1_intr_quirk;
	device->enumerate_channels = ioat1_enumerate_channels;
	device->self_test = ioat_dma_self_test;
	device->timer_fn = ioat1_timer_event;
	device->cleanup_fn = ioat1_cleanup_event;
	dma = &device->common;
	dma->device_prep_dma_memcpy = ioat1_dma_prep_memcpy;
	dma->device_issue_pending = ioat1_dma_memcpy_issue_pending;
	dma->device_alloc_chan_resources = ioat1_dma_alloc_chan_resources;
	dma->device_free_chan_resources = ioat1_dma_free_chan_resources;
	dma->device_tx_status = ioat_dma_tx_status;

	err = ioat_probe(device);
	if (err)
		return err;
	ioat_set_tcp_copy_break(4096);
	err = ioat_register(device);
	if (err)
		return err;
	ioat_kobject_add(device, &ioat1_ktype);

	if (dca)
		device->dca = ioat_dca_init(pdev, device->reg_base);

	return err;
}