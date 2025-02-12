static int proc_connectinfo(struct usb_dev_state *ps, void __user *arg)
{
	struct usbdevfs_connectinfo ci = {
		.devnum = ps->dev->devnum,
		.slow = ps->dev->speed == USB_SPEED_LOW
	};

	if (copy_to_user(arg, &ci, sizeof(ci)))
		return -EFAULT;
	return 0;
}