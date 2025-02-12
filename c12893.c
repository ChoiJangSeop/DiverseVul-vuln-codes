static int command_write(struct pci_dev *dev, int offset, u16 value, void *data)
{
	struct xen_pcibk_dev_data *dev_data;
	int err;

	dev_data = pci_get_drvdata(dev);
	if (!pci_is_enabled(dev) && is_enable_cmd(value)) {
		if (unlikely(verbose_request))
			printk(KERN_DEBUG DRV_NAME ": %s: enable\n",
			       pci_name(dev));
		err = pci_enable_device(dev);
		if (err)
			return err;
		if (dev_data)
			dev_data->enable_intx = 1;
	} else if (pci_is_enabled(dev) && !is_enable_cmd(value)) {
		if (unlikely(verbose_request))
			printk(KERN_DEBUG DRV_NAME ": %s: disable\n",
			       pci_name(dev));
		pci_disable_device(dev);
		if (dev_data)
			dev_data->enable_intx = 0;
	}

	if (!dev->is_busmaster && is_master_cmd(value)) {
		if (unlikely(verbose_request))
			printk(KERN_DEBUG DRV_NAME ": %s: set bus master\n",
			       pci_name(dev));
		pci_set_master(dev);
	}

	if (value & PCI_COMMAND_INVALIDATE) {
		if (unlikely(verbose_request))
			printk(KERN_DEBUG
			       DRV_NAME ": %s: enable memory-write-invalidate\n",
			       pci_name(dev));
		err = pci_set_mwi(dev);
		if (err) {
			pr_warn("%s: cannot enable memory-write-invalidate (%d)\n",
				pci_name(dev), err);
			value &= ~PCI_COMMAND_INVALIDATE;
		}
	}

	return pci_write_config_word(dev, offset, value);
}