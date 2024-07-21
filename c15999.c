pci_lintr_deassert(struct pci_vdev *dev)
{
	assert(dev->lintr.pin > 0);

	pthread_mutex_lock(&dev->lintr.lock);
	if (dev->lintr.state == ASSERTED) {
		dev->lintr.state = IDLE;
		pci_irq_deassert(dev);
	} else if (dev->lintr.state == PENDING)
		dev->lintr.state = IDLE;
	pthread_mutex_unlock(&dev->lintr.lock);
}