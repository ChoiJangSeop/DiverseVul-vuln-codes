static ssize_t k90_show_current_profile(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int ret;
	struct usb_interface *usbif = to_usb_interface(dev->parent);
	struct usb_device *usbdev = interface_to_usbdev(usbif);
	int current_profile;
	char data[8];

	ret = usb_control_msg(usbdev, usb_rcvctrlpipe(usbdev, 0),
			      K90_REQUEST_STATUS,
			      USB_DIR_IN | USB_TYPE_VENDOR |
			      USB_RECIP_DEVICE, 0, 0, data, 8,
			      USB_CTRL_SET_TIMEOUT);
	if (ret < 0) {
		dev_warn(dev, "Failed to get K90 initial state (error %d).\n",
			 ret);
		return -EIO;
	}
	current_profile = data[7];
	if (current_profile < 1 || current_profile > 3) {
		dev_warn(dev, "Read invalid current profile: %02hhx.\n",
			 data[7]);
		return -EIO;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", current_profile);
}