static void usbtest_disconnect(struct usb_interface *intf)
{
	struct usbtest_dev	*dev = usb_get_intfdata(intf);

	usb_set_intfdata(intf, NULL);
	dev_dbg(&intf->dev, "disconnect\n");
	kfree(dev);
}