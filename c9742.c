printer_open(struct inode *inode, struct file *fd)
{
	struct printer_dev	*dev;
	unsigned long		flags;
	int			ret = -EBUSY;

	dev = container_of(inode->i_cdev, struct printer_dev, printer_cdev);

	spin_lock_irqsave(&dev->lock, flags);

	if (dev->interface < 0) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return -ENODEV;
	}

	if (!dev->printer_cdev_open) {
		dev->printer_cdev_open = 1;
		fd->private_data = dev;
		ret = 0;
		/* Change the printer status to show that it's on-line. */
		dev->printer_status |= PRINTER_SELECTED;
	}

	spin_unlock_irqrestore(&dev->lock, flags);

	DBG(dev, "printer_open returned %x\n", ret);
	return ret;
}