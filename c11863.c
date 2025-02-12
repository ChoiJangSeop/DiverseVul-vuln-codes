static int em28xx_usb_probe(struct usb_interface *intf,
			    const struct usb_device_id *id)
{
	struct usb_device *udev;
	struct em28xx *dev = NULL;
	int retval;
	bool has_vendor_audio = false, has_video = false, has_dvb = false;
	int i, nr, try_bulk;
	const int ifnum = intf->altsetting[0].desc.bInterfaceNumber;
	char *speed;

	udev = usb_get_dev(interface_to_usbdev(intf));

	/* Check to see next free device and mark as used */
	do {
		nr = find_first_zero_bit(em28xx_devused, EM28XX_MAXBOARDS);
		if (nr >= EM28XX_MAXBOARDS) {
			/* No free device slots */
			dev_err(&intf->dev,
				"Driver supports up to %i em28xx boards.\n",
			       EM28XX_MAXBOARDS);
			retval = -ENOMEM;
			goto err_no_slot;
		}
	} while (test_and_set_bit(nr, em28xx_devused));

	/* Don't register audio interfaces */
	if (intf->altsetting[0].desc.bInterfaceClass == USB_CLASS_AUDIO) {
		dev_info(&intf->dev,
			"audio device (%04x:%04x): interface %i, class %i\n",
			le16_to_cpu(udev->descriptor.idVendor),
			le16_to_cpu(udev->descriptor.idProduct),
			ifnum,
			intf->altsetting[0].desc.bInterfaceClass);

		retval = -ENODEV;
		goto err;
	}

	/* allocate memory for our device state and initialize it */
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		retval = -ENOMEM;
		goto err;
	}

	/* compute alternate max packet sizes */
	dev->alt_max_pkt_size_isoc = kcalloc(intf->num_altsetting,
					     sizeof(dev->alt_max_pkt_size_isoc[0]),
					     GFP_KERNEL);
	if (!dev->alt_max_pkt_size_isoc) {
		kfree(dev);
		retval = -ENOMEM;
		goto err;
	}

	/* Get endpoints */
	for (i = 0; i < intf->num_altsetting; i++) {
		int ep;

		for (ep = 0;
		     ep < intf->altsetting[i].desc.bNumEndpoints;
		     ep++)
			em28xx_check_usb_descriptor(dev, udev, intf,
						    i, ep,
						    &has_vendor_audio,
						    &has_video,
						    &has_dvb);
	}

	if (!(has_vendor_audio || has_video || has_dvb)) {
		retval = -ENODEV;
		goto err_free;
	}

	switch (udev->speed) {
	case USB_SPEED_LOW:
		speed = "1.5";
		break;
	case USB_SPEED_UNKNOWN:
	case USB_SPEED_FULL:
		speed = "12";
		break;
	case USB_SPEED_HIGH:
		speed = "480";
		break;
	default:
		speed = "unknown";
	}

	dev_info(&intf->dev,
		"New device %s %s @ %s Mbps (%04x:%04x, interface %d, class %d)\n",
		udev->manufacturer ? udev->manufacturer : "",
		udev->product ? udev->product : "",
		speed,
		le16_to_cpu(udev->descriptor.idVendor),
		le16_to_cpu(udev->descriptor.idProduct),
		ifnum,
		intf->altsetting->desc.bInterfaceNumber);

	/*
	 * Make sure we have 480 Mbps of bandwidth, otherwise things like
	 * video stream wouldn't likely work, since 12 Mbps is generally
	 * not enough even for most Digital TV streams.
	 */
	if (udev->speed != USB_SPEED_HIGH && disable_usb_speed_check == 0) {
		dev_err(&intf->dev, "Device initialization failed.\n");
		dev_err(&intf->dev,
			"Device must be connected to a high-speed USB 2.0 port.\n");
		retval = -ENODEV;
		goto err_free;
	}

	dev->devno = nr;
	dev->model = id->driver_info;
	dev->alt   = -1;
	dev->is_audio_only = has_vendor_audio && !(has_video || has_dvb);
	dev->has_video = has_video;
	dev->ifnum = ifnum;

	dev->ts = PRIMARY_TS;
	snprintf(dev->name, 28, "em28xx");
	dev->dev_next = NULL;

	if (has_vendor_audio) {
		dev_info(&intf->dev,
			"Audio interface %i found (Vendor Class)\n", ifnum);
		dev->usb_audio_type = EM28XX_USB_AUDIO_VENDOR;
	}
	/* Checks if audio is provided by a USB Audio Class intf */
	for (i = 0; i < udev->config->desc.bNumInterfaces; i++) {
		struct usb_interface *uif = udev->config->interface[i];

		if (uif->altsetting[0].desc.bInterfaceClass == USB_CLASS_AUDIO) {
			if (has_vendor_audio)
				dev_err(&intf->dev,
					"em28xx: device seems to have vendor AND usb audio class interfaces !\n"
					"\t\tThe vendor interface will be ignored. Please contact the developers <linux-media@vger.kernel.org>\n");
			dev->usb_audio_type = EM28XX_USB_AUDIO_CLASS;
			break;
		}
	}

	if (has_video)
		dev_info(&intf->dev, "Video interface %i found:%s%s\n",
			ifnum,
			dev->analog_ep_bulk ? " bulk" : "",
			dev->analog_ep_isoc ? " isoc" : "");
	if (has_dvb)
		dev_info(&intf->dev, "DVB interface %i found:%s%s\n",
			ifnum,
			dev->dvb_ep_bulk ? " bulk" : "",
			dev->dvb_ep_isoc ? " isoc" : "");

	dev->num_alt = intf->num_altsetting;

	if ((unsigned int)card[nr] < em28xx_bcount)
		dev->model = card[nr];

	/* save our data pointer in this intf device */
	usb_set_intfdata(intf, dev);

	/* allocate device struct and check if the device is a webcam */
	mutex_init(&dev->lock);
	retval = em28xx_init_dev(dev, udev, intf, nr);
	if (retval)
		goto err_free;

	if (usb_xfer_mode < 0) {
		if (dev->is_webcam)
			try_bulk = 1;
		else
			try_bulk = 0;
	} else {
		try_bulk = usb_xfer_mode > 0;
	}

	/* Disable V4L2 if the device doesn't have a decoder or image sensor */
	if (has_video &&
	    dev->board.decoder == EM28XX_NODECODER &&
	    dev->em28xx_sensor == EM28XX_NOSENSOR) {
		dev_err(&intf->dev,
			"Currently, V4L2 is not supported on this model\n");
		has_video = false;
		dev->has_video = false;
	}

	if (dev->board.has_dual_ts &&
	    (dev->tuner_type != TUNER_ABSENT || INPUT(0)->type)) {
		/*
		 * The logic with sets alternate is not ready for dual-tuners
		 * which analog modes.
		 */
		dev_err(&intf->dev,
			"We currently don't support analog TV or stream capture on dual tuners.\n");
		has_video = false;
	}

	/* Select USB transfer types to use */
	if (has_video) {
		if (!dev->analog_ep_isoc || (try_bulk && dev->analog_ep_bulk))
			dev->analog_xfer_bulk = 1;
		dev_info(&intf->dev, "analog set to %s mode.\n",
			dev->analog_xfer_bulk ? "bulk" : "isoc");
	}
	if (has_dvb) {
		if (!dev->dvb_ep_isoc || (try_bulk && dev->dvb_ep_bulk))
			dev->dvb_xfer_bulk = 1;
		dev_info(&intf->dev, "dvb set to %s mode.\n",
			dev->dvb_xfer_bulk ? "bulk" : "isoc");
	}

	if (dev->board.has_dual_ts && em28xx_duplicate_dev(dev) == 0) {
		dev->dev_next->ts = SECONDARY_TS;
		dev->dev_next->alt   = -1;
		dev->dev_next->is_audio_only = has_vendor_audio &&
						!(has_video || has_dvb);
		dev->dev_next->has_video = false;
		dev->dev_next->ifnum = ifnum;
		dev->dev_next->model = id->driver_info;

		mutex_init(&dev->dev_next->lock);
		retval = em28xx_init_dev(dev->dev_next, udev, intf,
					 dev->dev_next->devno);
		if (retval)
			goto err_free;

		dev->dev_next->board.ir_codes = NULL; /* No IR for 2nd tuner */
		dev->dev_next->board.has_ir_i2c = 0; /* No IR for 2nd tuner */

		if (usb_xfer_mode < 0) {
			if (dev->dev_next->is_webcam)
				try_bulk = 1;
			else
				try_bulk = 0;
		} else {
			try_bulk = usb_xfer_mode > 0;
		}

		/* Select USB transfer types to use */
		if (has_dvb) {
			if (!dev->dvb_ep_isoc_ts2 ||
			    (try_bulk && dev->dvb_ep_bulk_ts2))
				dev->dev_next->dvb_xfer_bulk = 1;
			dev_info(&dev->intf->dev, "dvb ts2 set to %s mode.\n",
				 dev->dev_next->dvb_xfer_bulk ? "bulk" : "isoc");
		}

		dev->dev_next->dvb_ep_isoc = dev->dvb_ep_isoc_ts2;
		dev->dev_next->dvb_ep_bulk = dev->dvb_ep_bulk_ts2;
		dev->dev_next->dvb_max_pkt_size_isoc = dev->dvb_max_pkt_size_isoc_ts2;
		dev->dev_next->dvb_alt_isoc = dev->dvb_alt_isoc;

		/* Configure hardware to support TS2*/
		if (dev->dvb_xfer_bulk) {
			/* The ep4 and ep5 are configured for BULK */
			em28xx_write_reg(dev, 0x0b, 0x96);
			mdelay(100);
			em28xx_write_reg(dev, 0x0b, 0x80);
			mdelay(100);
		} else {
			/* The ep4 and ep5 are configured for ISO */
			em28xx_write_reg(dev, 0x0b, 0x96);
			mdelay(100);
			em28xx_write_reg(dev, 0x0b, 0x82);
			mdelay(100);
		}

		kref_init(&dev->dev_next->ref);
	}

	kref_init(&dev->ref);

	request_modules(dev);

	/*
	 * Do it at the end, to reduce dynamic configuration changes during
	 * the device init. Yet, as request_modules() can be async, the
	 * topology will likely change after the load of the em28xx subdrivers.
	 */
#ifdef CONFIG_MEDIA_CONTROLLER
	retval = media_device_register(dev->media_dev);
#endif

	return 0;

err_free:
	kfree(dev->alt_max_pkt_size_isoc);
	kfree(dev);

err:
	clear_bit(nr, em28xx_devused);

err_no_slot:
	usb_put_dev(udev);
	return retval;
}