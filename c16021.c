pci_lintr_assert(struct pci_vdev *dev)
{
	assert(dev->lintr.pin > 0);

	pthread_mutex_lock(&dev->lintr.lock);
	if (dev->lintr.state == IDLE) {
		if (pci_lintr_permitted(dev)) {
			dev->lintr.state = ASSERTED;
			pci_irq_assert(dev);
		} else
			dev->lintr.state = PENDING;
	}
	pthread_mutex_unlock(&dev->lintr.lock);
}