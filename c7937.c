int line6_probe(struct usb_interface *interface,
		const struct usb_device_id *id,
		const char *driver_name,
		const struct line6_properties *properties,
		int (*private_init)(struct usb_line6 *, const struct usb_device_id *id),
		size_t data_size)
{
	struct usb_device *usbdev = interface_to_usbdev(interface);
	struct snd_card *card;
	struct usb_line6 *line6;
	int interface_number;
	int ret;

	if (WARN_ON(data_size < sizeof(*line6)))
		return -EINVAL;

	/* we don't handle multiple configurations */
	if (usbdev->descriptor.bNumConfigurations != 1)
		return -ENODEV;

	ret = snd_card_new(&interface->dev,
			   SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1,
			   THIS_MODULE, data_size, &card);
	if (ret < 0)
		return ret;

	/* store basic data: */
	line6 = card->private_data;
	line6->card = card;
	line6->properties = properties;
	line6->usbdev = usbdev;
	line6->ifcdev = &interface->dev;

	strcpy(card->id, properties->id);
	strcpy(card->driver, driver_name);
	strcpy(card->shortname, properties->name);
	sprintf(card->longname, "Line 6 %s at USB %s", properties->name,
		dev_name(line6->ifcdev));
	card->private_free = line6_destruct;

	usb_set_intfdata(interface, line6);

	/* increment reference counters: */
	usb_get_dev(usbdev);

	/* initialize device info: */
	dev_info(&interface->dev, "Line 6 %s found\n", properties->name);

	/* query interface number */
	interface_number = interface->cur_altsetting->desc.bInterfaceNumber;

	/* TODO reserves the bus bandwidth even without actual transfer */
	ret = usb_set_interface(usbdev, interface_number,
				properties->altsetting);
	if (ret < 0) {
		dev_err(&interface->dev, "set_interface failed\n");
		goto error;
	}

	line6_get_usb_properties(line6);

	if (properties->capabilities & LINE6_CAP_CONTROL) {
		ret = line6_init_cap_control(line6);
		if (ret < 0)
			goto error;
	}

	/* initialize device data based on device: */
	ret = private_init(line6, id);
	if (ret < 0)
		goto error;

	/* creation of additional special files should go here */

	dev_info(&interface->dev, "Line 6 %s now attached\n",
		 properties->name);

	return 0;

 error:
	/* we can call disconnect callback here because no close-sync is
	 * needed yet at this point
	 */
	line6_disconnect(interface);
	return ret;
}