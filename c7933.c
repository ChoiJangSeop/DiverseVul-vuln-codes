static int toneport_init(struct usb_line6 *line6,
			 const struct usb_device_id *id)
{
	int err;
	struct usb_line6_toneport *toneport =  (struct usb_line6_toneport *) line6;

	toneport->type = id->driver_info;
	INIT_DELAYED_WORK(&toneport->pcm_work, toneport_start_pcm);

	line6->disconnect = line6_toneport_disconnect;

	/* initialize PCM subsystem: */
	err = line6_init_pcm(line6, &toneport_pcm_properties);
	if (err < 0)
		return err;

	/* register monitor control: */
	err = snd_ctl_add(line6->card,
			  snd_ctl_new1(&toneport_control_monitor,
				       line6->line6pcm));
	if (err < 0)
		return err;

	/* register source select control: */
	if (toneport_has_source_select(toneport)) {
		err =
		    snd_ctl_add(line6->card,
				snd_ctl_new1(&toneport_control_source,
					     line6->line6pcm));
		if (err < 0)
			return err;
	}

	line6_read_serial_number(line6, &toneport->serial_number);
	line6_read_data(line6, 0x80c2, &toneport->firmware_version, 1);

	if (toneport_has_led(toneport)) {
		err = toneport_init_leds(toneport);
		if (err < 0)
			return err;
	}

	err = toneport_setup(toneport);
	if (err)
		return err;

	/* register audio system: */
	return snd_card_register(line6->card);
}