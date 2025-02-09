static int hso_probe(struct usb_interface *interface,
		     const struct usb_device_id *id)
{
	int mux, i, if_num, port_spec;
	unsigned char port_mask;
	struct hso_device *hso_dev = NULL;
	struct hso_shared_int *shared_int;
	struct hso_device *tmp_dev = NULL;

	if (interface->cur_altsetting->desc.bInterfaceClass != 0xFF) {
		dev_err(&interface->dev, "Not our interface\n");
		return -ENODEV;
	}

	if_num = interface->cur_altsetting->desc.bInterfaceNumber;

	/* Get the interface/port specification from either driver_info or from
	 * the device itself */
	if (id->driver_info)
		port_spec = ((u32 *)(id->driver_info))[if_num];
	else
		port_spec = hso_get_config_data(interface);

	/* Check if we need to switch to alt interfaces prior to port
	 * configuration */
	if (interface->num_altsetting > 1)
		usb_set_interface(interface_to_usbdev(interface), if_num, 1);
	interface->needs_remote_wakeup = 1;

	/* Allocate new hso device(s) */
	switch (port_spec & HSO_INTF_MASK) {
	case HSO_INTF_MUX:
		if ((port_spec & HSO_PORT_MASK) == HSO_PORT_NETWORK) {
			/* Create the network device */
			if (!disable_net) {
				hso_dev = hso_create_net_device(interface,
								port_spec);
				if (!hso_dev)
					goto exit;
				tmp_dev = hso_dev;
			}
		}

		if (hso_get_mux_ports(interface, &port_mask))
			/* TODO: de-allocate everything */
			goto exit;

		shared_int = hso_create_shared_int(interface);
		if (!shared_int)
			goto exit;

		for (i = 1, mux = 0; i < 0x100; i = i << 1, mux++) {
			if (port_mask & i) {
				hso_dev = hso_create_mux_serial_device(
						interface, i, shared_int);
				if (!hso_dev)
					goto exit;
			}
		}

		if (tmp_dev)
			hso_dev = tmp_dev;
		break;

	case HSO_INTF_BULK:
		/* It's a regular bulk interface */
		if ((port_spec & HSO_PORT_MASK) == HSO_PORT_NETWORK) {
			if (!disable_net)
				hso_dev =
				    hso_create_net_device(interface, port_spec);
		} else {
			hso_dev =
			    hso_create_bulk_serial_device(interface, port_spec);
		}
		if (!hso_dev)
			goto exit;
		break;
	default:
		goto exit;
	}

	/* save our data pointer in this device */
	usb_set_intfdata(interface, hso_dev);

	/* done */
	return 0;
exit:
	hso_free_interface(interface);
	return -ENODEV;
}