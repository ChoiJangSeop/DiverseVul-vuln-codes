static int atusb_get_and_show_build(struct atusb *atusb)
{
	struct usb_device *usb_dev = atusb->usb_dev;
	char build[ATUSB_BUILD_SIZE + 1];
	int ret;

	ret = atusb_control_msg(atusb, usb_rcvctrlpipe(usb_dev, 0),
				ATUSB_BUILD, ATUSB_REQ_FROM_DEV, 0, 0,
				build, ATUSB_BUILD_SIZE, 1000);
	if (ret >= 0) {
		build[ret] = 0;
		dev_info(&usb_dev->dev, "Firmware: build %s\n", build);
	}

	return ret;
}