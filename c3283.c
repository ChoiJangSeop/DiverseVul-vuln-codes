aiptek_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *usbdev = interface_to_usbdev(intf);
	struct usb_endpoint_descriptor *endpoint;
	struct aiptek *aiptek;
	struct input_dev *inputdev;
	int i;
	int speeds[] = { 0,
		AIPTEK_PROGRAMMABLE_DELAY_50,
		AIPTEK_PROGRAMMABLE_DELAY_400,
		AIPTEK_PROGRAMMABLE_DELAY_25,
		AIPTEK_PROGRAMMABLE_DELAY_100,
		AIPTEK_PROGRAMMABLE_DELAY_200,
		AIPTEK_PROGRAMMABLE_DELAY_300
	};
	int err = -ENOMEM;

	/* programmableDelay is where the command-line specified
	 * delay is kept. We make it the first element of speeds[],
	 * so therefore, your override speed is tried first, then the
	 * remainder. Note that the default value of 400ms will be tried
	 * if you do not specify any command line parameter.
	 */
	speeds[0] = programmableDelay;

	aiptek = kzalloc(sizeof(struct aiptek), GFP_KERNEL);
	inputdev = input_allocate_device();
	if (!aiptek || !inputdev) {
		dev_warn(&intf->dev,
			 "cannot allocate memory or input device\n");
		goto fail1;
        }

	aiptek->data = usb_alloc_coherent(usbdev, AIPTEK_PACKET_LENGTH,
					  GFP_ATOMIC, &aiptek->data_dma);
        if (!aiptek->data) {
		dev_warn(&intf->dev, "cannot allocate usb buffer\n");
		goto fail1;
	}

	aiptek->urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!aiptek->urb) {
	        dev_warn(&intf->dev, "cannot allocate urb\n");
		goto fail2;
	}

	aiptek->inputdev = inputdev;
	aiptek->usbdev = usbdev;
	aiptek->intf = intf;
	aiptek->ifnum = intf->altsetting[0].desc.bInterfaceNumber;
	aiptek->inDelay = 0;
	aiptek->endDelay = 0;
	aiptek->previousJitterable = 0;
	aiptek->lastMacro = -1;

	/* Set up the curSettings struct. Said struct contains the current
	 * programmable parameters. The newSetting struct contains changes
	 * the user makes to the settings via the sysfs interface. Those
	 * changes are not "committed" to curSettings until the user
	 * writes to the sysfs/.../execute file.
	 */
	aiptek->curSetting.pointerMode = AIPTEK_POINTER_EITHER_MODE;
	aiptek->curSetting.coordinateMode = AIPTEK_COORDINATE_ABSOLUTE_MODE;
	aiptek->curSetting.toolMode = AIPTEK_TOOL_BUTTON_PEN_MODE;
	aiptek->curSetting.xTilt = AIPTEK_TILT_DISABLE;
	aiptek->curSetting.yTilt = AIPTEK_TILT_DISABLE;
	aiptek->curSetting.mouseButtonLeft = AIPTEK_MOUSE_LEFT_BUTTON;
	aiptek->curSetting.mouseButtonMiddle = AIPTEK_MOUSE_MIDDLE_BUTTON;
	aiptek->curSetting.mouseButtonRight = AIPTEK_MOUSE_RIGHT_BUTTON;
	aiptek->curSetting.stylusButtonUpper = AIPTEK_STYLUS_UPPER_BUTTON;
	aiptek->curSetting.stylusButtonLower = AIPTEK_STYLUS_LOWER_BUTTON;
	aiptek->curSetting.jitterDelay = jitterDelay;
	aiptek->curSetting.programmableDelay = programmableDelay;

	/* Both structs should have equivalent settings
	 */
	aiptek->newSetting = aiptek->curSetting;

	/* Determine the usb devices' physical path.
	 * Asketh not why we always pretend we're using "../input0",
	 * but I suspect this will have to be refactored one
	 * day if a single USB device can be a keyboard & a mouse
	 * & a tablet, and the inputX number actually will tell
	 * us something...
	 */
	usb_make_path(usbdev, aiptek->features.usbPath,
			sizeof(aiptek->features.usbPath));
	strlcat(aiptek->features.usbPath, "/input0",
		sizeof(aiptek->features.usbPath));

	/* Set up client data, pointers to open and close routines
	 * for the input device.
	 */
	inputdev->name = "Aiptek";
	inputdev->phys = aiptek->features.usbPath;
	usb_to_input_id(usbdev, &inputdev->id);
	inputdev->dev.parent = &intf->dev;

	input_set_drvdata(inputdev, aiptek);

	inputdev->open = aiptek_open;
	inputdev->close = aiptek_close;

	/* Now program the capacities of the tablet, in terms of being
	 * an input device.
	 */
	for (i = 0; i < ARRAY_SIZE(eventTypes); ++i)
	        __set_bit(eventTypes[i], inputdev->evbit);

	for (i = 0; i < ARRAY_SIZE(absEvents); ++i)
	        __set_bit(absEvents[i], inputdev->absbit);

	for (i = 0; i < ARRAY_SIZE(relEvents); ++i)
	        __set_bit(relEvents[i], inputdev->relbit);

	__set_bit(MSC_SERIAL, inputdev->mscbit);

	/* Set up key and button codes */
	for (i = 0; i < ARRAY_SIZE(buttonEvents); ++i)
		__set_bit(buttonEvents[i], inputdev->keybit);

	for (i = 0; i < ARRAY_SIZE(macroKeyEvents); ++i)
		__set_bit(macroKeyEvents[i], inputdev->keybit);

	/*
	 * Program the input device coordinate capacities. We do not yet
	 * know what maximum X, Y, and Z values are, so we're putting fake
	 * values in. Later, we'll ask the tablet to put in the correct
	 * values.
	 */
	input_set_abs_params(inputdev, ABS_X, 0, 2999, 0, 0);
	input_set_abs_params(inputdev, ABS_Y, 0, 2249, 0, 0);
	input_set_abs_params(inputdev, ABS_PRESSURE, 0, 511, 0, 0);
	input_set_abs_params(inputdev, ABS_TILT_X, AIPTEK_TILT_MIN, AIPTEK_TILT_MAX, 0, 0);
	input_set_abs_params(inputdev, ABS_TILT_Y, AIPTEK_TILT_MIN, AIPTEK_TILT_MAX, 0, 0);
	input_set_abs_params(inputdev, ABS_WHEEL, AIPTEK_WHEEL_MIN, AIPTEK_WHEEL_MAX - 1, 0, 0);

	endpoint = &intf->altsetting[0].endpoint[0].desc;

	/* Go set up our URB, which is called when the tablet receives
	 * input.
	 */
	usb_fill_int_urb(aiptek->urb,
			 aiptek->usbdev,
			 usb_rcvintpipe(aiptek->usbdev,
					endpoint->bEndpointAddress),
			 aiptek->data, 8, aiptek_irq, aiptek,
			 endpoint->bInterval);

	aiptek->urb->transfer_dma = aiptek->data_dma;
	aiptek->urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	/* Program the tablet. This sets the tablet up in the mode
	 * specified in newSetting, and also queries the tablet's
	 * physical capacities.
	 *
	 * Sanity check: if a tablet doesn't like the slow programmatic
	 * delay, we often get sizes of 0x0. Let's use that as an indicator
	 * to try faster delays, up to 25 ms. If that logic fails, well, you'll
	 * have to explain to us how your tablet thinks it's 0x0, and yet that's
	 * not an error :-)
	 */

	for (i = 0; i < ARRAY_SIZE(speeds); ++i) {
		aiptek->curSetting.programmableDelay = speeds[i];
		(void)aiptek_program_tablet(aiptek);
		if (input_abs_get_max(aiptek->inputdev, ABS_X) > 0) {
			dev_info(&intf->dev,
				 "Aiptek using %d ms programming speed\n",
				 aiptek->curSetting.programmableDelay);
			break;
		}
	}

	/* Murphy says that some day someone will have a tablet that fails the
	   above test. That's you, Frederic Rodrigo */
	if (i == ARRAY_SIZE(speeds)) {
		dev_info(&intf->dev,
			 "Aiptek tried all speeds, no sane response\n");
		goto fail3;
	}

	/* Associate this driver's struct with the usb interface.
	 */
	usb_set_intfdata(intf, aiptek);

	/* Set up the sysfs files
	 */
	err = sysfs_create_group(&intf->dev.kobj, &aiptek_attribute_group);
	if (err) {
		dev_warn(&intf->dev, "cannot create sysfs group err: %d\n",
			 err);
		goto fail3;
        }

	/* Register the tablet as an Input Device
	 */
	err = input_register_device(aiptek->inputdev);
	if (err) {
		dev_warn(&intf->dev,
			 "input_register_device returned err: %d\n", err);
		goto fail4;
        }
	return 0;

 fail4:	sysfs_remove_group(&intf->dev.kobj, &aiptek_attribute_group);
 fail3: usb_free_urb(aiptek->urb);
 fail2:	usb_free_coherent(usbdev, AIPTEK_PACKET_LENGTH, aiptek->data,
			  aiptek->data_dma);
 fail1: usb_set_intfdata(intf, NULL);
	input_free_device(inputdev);
	kfree(aiptek);
	return err;
}