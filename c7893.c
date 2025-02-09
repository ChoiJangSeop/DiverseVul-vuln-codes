static int sisusb_probe(struct usb_interface *intf,
		const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct sisusb_usb_data *sisusb;
	int retval = 0, i;

	dev_info(&dev->dev, "USB2VGA dongle found at address %d\n",
			dev->devnum);

	/* Allocate memory for our private */
	sisusb = kzalloc(sizeof(*sisusb), GFP_KERNEL);
	if (!sisusb)
		return -ENOMEM;

	kref_init(&sisusb->kref);

	mutex_init(&(sisusb->lock));

	/* Register device */
	retval = usb_register_dev(intf, &usb_sisusb_class);
	if (retval) {
		dev_err(&sisusb->sisusb_dev->dev,
				"Failed to get a minor for device %d\n",
				dev->devnum);
		retval = -ENODEV;
		goto error_1;
	}

	sisusb->sisusb_dev = dev;
	sisusb->minor      = intf->minor;
	sisusb->vrambase   = SISUSB_PCI_MEMBASE;
	sisusb->mmiobase   = SISUSB_PCI_MMIOBASE;
	sisusb->mmiosize   = SISUSB_PCI_MMIOSIZE;
	sisusb->ioportbase = SISUSB_PCI_IOPORTBASE;
	/* Everything else is zero */

	/* Allocate buffers */
	sisusb->ibufsize = SISUSB_IBUF_SIZE;
	sisusb->ibuf = kmalloc(SISUSB_IBUF_SIZE, GFP_KERNEL);
	if (!sisusb->ibuf) {
		retval = -ENOMEM;
		goto error_2;
	}

	sisusb->numobufs = 0;
	sisusb->obufsize = SISUSB_OBUF_SIZE;
	for (i = 0; i < NUMOBUFS; i++) {
		sisusb->obuf[i] = kmalloc(SISUSB_OBUF_SIZE, GFP_KERNEL);
		if (!sisusb->obuf[i]) {
			if (i == 0) {
				retval = -ENOMEM;
				goto error_3;
			}
			break;
		}
		sisusb->numobufs++;
	}

	/* Allocate URBs */
	sisusb->sisurbin = usb_alloc_urb(0, GFP_KERNEL);
	if (!sisusb->sisurbin) {
		retval = -ENOMEM;
		goto error_3;
	}
	sisusb->completein = 1;

	for (i = 0; i < sisusb->numobufs; i++) {
		sisusb->sisurbout[i] = usb_alloc_urb(0, GFP_KERNEL);
		if (!sisusb->sisurbout[i]) {
			retval = -ENOMEM;
			goto error_4;
		}
		sisusb->urbout_context[i].sisusb = (void *)sisusb;
		sisusb->urbout_context[i].urbindex = i;
		sisusb->urbstatus[i] = 0;
	}

	dev_info(&sisusb->sisusb_dev->dev, "Allocated %d output buffers\n",
			sisusb->numobufs);

#ifdef CONFIG_USB_SISUSBVGA_CON
	/* Allocate our SiS_Pr */
	sisusb->SiS_Pr = kmalloc(sizeof(struct SiS_Private), GFP_KERNEL);
	if (!sisusb->SiS_Pr) {
		retval = -ENOMEM;
		goto error_4;
	}
#endif

	/* Do remaining init stuff */

	init_waitqueue_head(&sisusb->wait_q);

	usb_set_intfdata(intf, sisusb);

	usb_get_dev(sisusb->sisusb_dev);

	sisusb->present = 1;

	if (dev->speed == USB_SPEED_HIGH || dev->speed >= USB_SPEED_SUPER) {
		int initscreen = 1;
#ifdef CONFIG_USB_SISUSBVGA_CON
		if (sisusb_first_vc > 0 && sisusb_last_vc > 0 &&
				sisusb_first_vc <= sisusb_last_vc &&
				sisusb_last_vc <= MAX_NR_CONSOLES)
			initscreen = 0;
#endif
		if (sisusb_init_gfxdevice(sisusb, initscreen))
			dev_err(&sisusb->sisusb_dev->dev,
					"Failed to early initialize device\n");

	} else
		dev_info(&sisusb->sisusb_dev->dev,
				"Not attached to USB 2.0 hub, deferring init\n");

	sisusb->ready = 1;

#ifdef SISUSBENDIANTEST
	dev_dbg(&sisusb->sisusb_dev->dev, "*** RWTEST ***\n");
	sisusb_testreadwrite(sisusb);
	dev_dbg(&sisusb->sisusb_dev->dev, "*** RWTEST END ***\n");
#endif

#ifdef CONFIG_USB_SISUSBVGA_CON
	sisusb_console_init(sisusb, sisusb_first_vc, sisusb_last_vc);
#endif

	return 0;

error_4:
	sisusb_free_urbs(sisusb);
error_3:
	sisusb_free_buffers(sisusb);
error_2:
	usb_deregister_dev(intf, &usb_sisusb_class);
error_1:
	kfree(sisusb);
	return retval;
}