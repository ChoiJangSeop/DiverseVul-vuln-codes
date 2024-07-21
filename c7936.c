void line6_disconnect(struct usb_interface *interface)
{
	struct usb_line6 *line6 = usb_get_intfdata(interface);
	struct usb_device *usbdev = interface_to_usbdev(interface);

	if (!line6)
		return;

	if (WARN_ON(usbdev != line6->usbdev))
		return;

	if (line6->urb_listen != NULL)
		line6_stop_listen(line6);

	snd_card_disconnect(line6->card);
	if (line6->line6pcm)
		line6_pcm_disconnect(line6->line6pcm);
	if (line6->disconnect)
		line6->disconnect(line6);

	dev_info(&interface->dev, "Line 6 %s now disconnected\n",
		 line6->properties->name);

	/* make sure the device isn't destructed twice: */
	usb_set_intfdata(interface, NULL);

	snd_card_free_when_closed(line6->card);
}