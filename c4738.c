static int atusb_read_reg(struct atusb *atusb, uint8_t reg)
{
	struct usb_device *usb_dev = atusb->usb_dev;
	int ret;
	uint8_t value;

	dev_dbg(&usb_dev->dev, "atusb: reg = 0x%x\n", reg);
	ret = atusb_control_msg(atusb, usb_rcvctrlpipe(usb_dev, 0),
				ATUSB_REG_READ, ATUSB_REQ_FROM_DEV,
				0, reg, &value, 1, 1000);
	return ret >= 0 ? value : ret;
}