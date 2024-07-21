printer_close(struct inode *inode, struct file *fd)
{
	struct printer_dev	*dev = fd->private_data;
	unsigned long		flags;

	spin_lock_irqsave(&dev->lock, flags);
	dev->printer_cdev_open = 0;
	fd->private_data = NULL;
	/* Change printer status to show that the printer is off-line. */
	dev->printer_status &= ~PRINTER_SELECTED;
	spin_unlock_irqrestore(&dev->lock, flags);

	DBG(dev, "printer_close\n");

	return 0;
}