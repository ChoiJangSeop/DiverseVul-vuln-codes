int snd_usbmidi_create(struct snd_card *card,
		       struct usb_interface *iface,
		       struct list_head *midi_list,
		       const struct snd_usb_audio_quirk *quirk)
{
	struct snd_usb_midi *umidi;
	struct snd_usb_midi_endpoint_info endpoints[MIDI_MAX_ENDPOINTS];
	int out_ports, in_ports;
	int i, err;

	umidi = kzalloc(sizeof(*umidi), GFP_KERNEL);
	if (!umidi)
		return -ENOMEM;
	umidi->dev = interface_to_usbdev(iface);
	umidi->card = card;
	umidi->iface = iface;
	umidi->quirk = quirk;
	umidi->usb_protocol_ops = &snd_usbmidi_standard_ops;
	spin_lock_init(&umidi->disc_lock);
	init_rwsem(&umidi->disc_rwsem);
	mutex_init(&umidi->mutex);
	umidi->usb_id = USB_ID(le16_to_cpu(umidi->dev->descriptor.idVendor),
			       le16_to_cpu(umidi->dev->descriptor.idProduct));
	setup_timer(&umidi->error_timer, snd_usbmidi_error_timer,
		    (unsigned long)umidi);

	/* detect the endpoint(s) to use */
	memset(endpoints, 0, sizeof(endpoints));
	switch (quirk ? quirk->type : QUIRK_MIDI_STANDARD_INTERFACE) {
	case QUIRK_MIDI_STANDARD_INTERFACE:
		err = snd_usbmidi_get_ms_info(umidi, endpoints);
		if (umidi->usb_id == USB_ID(0x0763, 0x0150)) /* M-Audio Uno */
			umidi->usb_protocol_ops =
				&snd_usbmidi_maudio_broken_running_status_ops;
		break;
	case QUIRK_MIDI_US122L:
		umidi->usb_protocol_ops = &snd_usbmidi_122l_ops;
		/* fall through */
	case QUIRK_MIDI_FIXED_ENDPOINT:
		memcpy(&endpoints[0], quirk->data,
		       sizeof(struct snd_usb_midi_endpoint_info));
		err = snd_usbmidi_detect_endpoints(umidi, &endpoints[0], 1);
		break;
	case QUIRK_MIDI_YAMAHA:
		err = snd_usbmidi_detect_yamaha(umidi, &endpoints[0]);
		break;
	case QUIRK_MIDI_ROLAND:
		err = snd_usbmidi_detect_roland(umidi, &endpoints[0]);
		break;
	case QUIRK_MIDI_MIDIMAN:
		umidi->usb_protocol_ops = &snd_usbmidi_midiman_ops;
		memcpy(&endpoints[0], quirk->data,
		       sizeof(struct snd_usb_midi_endpoint_info));
		err = 0;
		break;
	case QUIRK_MIDI_NOVATION:
		umidi->usb_protocol_ops = &snd_usbmidi_novation_ops;
		err = snd_usbmidi_detect_per_port_endpoints(umidi, endpoints);
		break;
	case QUIRK_MIDI_RAW_BYTES:
		umidi->usb_protocol_ops = &snd_usbmidi_raw_ops;
		/*
		 * Interface 1 contains isochronous endpoints, but with the same
		 * numbers as in interface 0.  Since it is interface 1 that the
		 * USB core has most recently seen, these descriptors are now
		 * associated with the endpoint numbers.  This will foul up our
		 * attempts to submit bulk/interrupt URBs to the endpoints in
		 * interface 0, so we have to make sure that the USB core looks
		 * again at interface 0 by calling usb_set_interface() on it.
		 */
		if (umidi->usb_id == USB_ID(0x07fd, 0x0001)) /* MOTU Fastlane */
			usb_set_interface(umidi->dev, 0, 0);
		err = snd_usbmidi_detect_per_port_endpoints(umidi, endpoints);
		break;
	case QUIRK_MIDI_EMAGIC:
		umidi->usb_protocol_ops = &snd_usbmidi_emagic_ops;
		memcpy(&endpoints[0], quirk->data,
		       sizeof(struct snd_usb_midi_endpoint_info));
		err = snd_usbmidi_detect_endpoints(umidi, &endpoints[0], 1);
		break;
	case QUIRK_MIDI_CME:
		umidi->usb_protocol_ops = &snd_usbmidi_cme_ops;
		err = snd_usbmidi_detect_per_port_endpoints(umidi, endpoints);
		break;
	case QUIRK_MIDI_AKAI:
		umidi->usb_protocol_ops = &snd_usbmidi_akai_ops;
		err = snd_usbmidi_detect_per_port_endpoints(umidi, endpoints);
		/* endpoint 1 is input-only */
		endpoints[1].out_cables = 0;
		break;
	case QUIRK_MIDI_FTDI:
		umidi->usb_protocol_ops = &snd_usbmidi_ftdi_ops;

		/* set baud rate to 31250 (48 MHz / 16 / 96) */
		err = usb_control_msg(umidi->dev, usb_sndctrlpipe(umidi->dev, 0),
				      3, 0x40, 0x60, 0, NULL, 0, 1000);
		if (err < 0)
			break;

		err = snd_usbmidi_detect_per_port_endpoints(umidi, endpoints);
		break;
	case QUIRK_MIDI_CH345:
		umidi->usb_protocol_ops = &snd_usbmidi_ch345_broken_sysex_ops;
		err = snd_usbmidi_detect_per_port_endpoints(umidi, endpoints);
		break;
	default:
		dev_err(&umidi->dev->dev, "invalid quirk type %d\n",
			quirk->type);
		err = -ENXIO;
		break;
	}
	if (err < 0) {
		kfree(umidi);
		return err;
	}

	/* create rawmidi device */
	out_ports = 0;
	in_ports = 0;
	for (i = 0; i < MIDI_MAX_ENDPOINTS; ++i) {
		out_ports += hweight16(endpoints[i].out_cables);
		in_ports += hweight16(endpoints[i].in_cables);
	}
	err = snd_usbmidi_create_rawmidi(umidi, out_ports, in_ports);
	if (err < 0) {
		kfree(umidi);
		return err;
	}

	/* create endpoint/port structures */
	if (quirk && quirk->type == QUIRK_MIDI_MIDIMAN)
		err = snd_usbmidi_create_endpoints_midiman(umidi, &endpoints[0]);
	else
		err = snd_usbmidi_create_endpoints(umidi, endpoints);
	if (err < 0) {
		snd_usbmidi_free(umidi);
		return err;
	}

	usb_autopm_get_interface_no_resume(umidi->iface);

	list_add_tail(&umidi->list, midi_list);
	return 0;
}