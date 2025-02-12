static int iowarrior_probe(struct usb_interface *interface,
			   const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	struct iowarrior *dev = NULL;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	int i;
	int retval = -ENOMEM;

	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(struct iowarrior), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&interface->dev, "Out of memory\n");
		return retval;
	}

	mutex_init(&dev->mutex);

	atomic_set(&dev->intr_idx, 0);
	atomic_set(&dev->read_idx, 0);
	spin_lock_init(&dev->intr_idx_lock);
	atomic_set(&dev->overflow_flag, 0);
	init_waitqueue_head(&dev->read_wait);
	atomic_set(&dev->write_busy, 0);
	init_waitqueue_head(&dev->write_wait);

	dev->udev = udev;
	dev->interface = interface;

	iface_desc = interface->cur_altsetting;
	dev->product_id = le16_to_cpu(udev->descriptor.idProduct);

	/* set up the endpoint information */
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		if (usb_endpoint_is_int_in(endpoint))
			dev->int_in_endpoint = endpoint;
		if (usb_endpoint_is_int_out(endpoint))
			/* this one will match for the IOWarrior56 only */
			dev->int_out_endpoint = endpoint;
	}
	/* we have to check the report_size often, so remember it in the endianness suitable for our machine */
	dev->report_size = usb_endpoint_maxp(dev->int_in_endpoint);
	if ((dev->interface->cur_altsetting->desc.bInterfaceNumber == 0) &&
	    (dev->product_id == USB_DEVICE_ID_CODEMERCS_IOW56))
		/* IOWarrior56 has wMaxPacketSize different from report size */
		dev->report_size = 7;

	/* create the urb and buffer for reading */
	dev->int_in_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!dev->int_in_urb) {
		dev_err(&interface->dev, "Couldn't allocate interrupt_in_urb\n");
		goto error;
	}
	dev->int_in_buffer = kmalloc(dev->report_size, GFP_KERNEL);
	if (!dev->int_in_buffer) {
		dev_err(&interface->dev, "Couldn't allocate int_in_buffer\n");
		goto error;
	}
	usb_fill_int_urb(dev->int_in_urb, dev->udev,
			 usb_rcvintpipe(dev->udev,
					dev->int_in_endpoint->bEndpointAddress),
			 dev->int_in_buffer, dev->report_size,
			 iowarrior_callback, dev,
			 dev->int_in_endpoint->bInterval);
	/* create an internal buffer for interrupt data from the device */
	dev->read_queue =
	    kmalloc(((dev->report_size + 1) * MAX_INTERRUPT_BUFFER),
		    GFP_KERNEL);
	if (!dev->read_queue) {
		dev_err(&interface->dev, "Couldn't allocate read_queue\n");
		goto error;
	}
	/* Get the serial-number of the chip */
	memset(dev->chip_serial, 0x00, sizeof(dev->chip_serial));
	usb_string(udev, udev->descriptor.iSerialNumber, dev->chip_serial,
		   sizeof(dev->chip_serial));
	if (strlen(dev->chip_serial) != 8)
		memset(dev->chip_serial, 0x00, sizeof(dev->chip_serial));

	/* Set the idle timeout to 0, if this is interface 0 */
	if (dev->interface->cur_altsetting->desc.bInterfaceNumber == 0) {
	    usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			    0x0A,
			    USB_TYPE_CLASS | USB_RECIP_INTERFACE, 0,
			    0, NULL, 0, USB_CTRL_SET_TIMEOUT);
	}
	/* allow device read and ioctl */
	dev->present = 1;

	/* we can register the device now, as it is ready */
	usb_set_intfdata(interface, dev);

	retval = usb_register_dev(interface, &iowarrior_class);
	if (retval) {
		/* something prevented us from registering this driver */
		dev_err(&interface->dev, "Not able to get a minor for this device.\n");
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	dev->minor = interface->minor;

	/* let the user know what node this device is now attached to */
	dev_info(&interface->dev, "IOWarrior product=0x%x, serial=%s interface=%d "
		 "now attached to iowarrior%d\n", dev->product_id, dev->chip_serial,
		 iface_desc->desc.bInterfaceNumber, dev->minor - IOWARRIOR_MINOR_BASE);
	return retval;

error:
	iowarrior_delete(dev);
	return retval;
}