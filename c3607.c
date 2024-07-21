static int create_uaxx_quirk(struct snd_usb_audio *chip,
			     struct usb_interface *iface,
			     struct usb_driver *driver,
			     const struct snd_usb_audio_quirk *quirk)
{
	static const struct audioformat ua_format = {
		.formats = SNDRV_PCM_FMTBIT_S24_3LE,
		.channels = 2,
		.fmt_type = UAC_FORMAT_TYPE_I,
		.altsetting = 1,
		.altset_idx = 1,
		.rates = SNDRV_PCM_RATE_CONTINUOUS,
	};
	struct usb_host_interface *alts;
	struct usb_interface_descriptor *altsd;
	struct audioformat *fp;
	int stream, err;

	/* both PCM and MIDI interfaces have 2 or more altsettings */
	if (iface->num_altsetting < 2)
		return -ENXIO;
	alts = &iface->altsetting[1];
	altsd = get_iface_desc(alts);

	if (altsd->bNumEndpoints == 2) {
		static const struct snd_usb_midi_endpoint_info ua700_ep = {
			.out_cables = 0x0003,
			.in_cables  = 0x0003
		};
		static const struct snd_usb_audio_quirk ua700_quirk = {
			.type = QUIRK_MIDI_FIXED_ENDPOINT,
			.data = &ua700_ep
		};
		static const struct snd_usb_midi_endpoint_info uaxx_ep = {
			.out_cables = 0x0001,
			.in_cables  = 0x0001
		};
		static const struct snd_usb_audio_quirk uaxx_quirk = {
			.type = QUIRK_MIDI_FIXED_ENDPOINT,
			.data = &uaxx_ep
		};
		const struct snd_usb_audio_quirk *quirk =
			chip->usb_id == USB_ID(0x0582, 0x002b)
			? &ua700_quirk : &uaxx_quirk;
		return __snd_usbmidi_create(chip->card, iface,
					  &chip->midi_list, quirk,
					  chip->usb_id);
	}

	if (altsd->bNumEndpoints != 1)
		return -ENXIO;

	fp = kmemdup(&ua_format, sizeof(*fp), GFP_KERNEL);
	if (!fp)
		return -ENOMEM;

	fp->iface = altsd->bInterfaceNumber;
	fp->endpoint = get_endpoint(alts, 0)->bEndpointAddress;
	fp->ep_attr = get_endpoint(alts, 0)->bmAttributes;
	fp->datainterval = 0;
	fp->maxpacksize = le16_to_cpu(get_endpoint(alts, 0)->wMaxPacketSize);

	switch (fp->maxpacksize) {
	case 0x120:
		fp->rate_max = fp->rate_min = 44100;
		break;
	case 0x138:
	case 0x140:
		fp->rate_max = fp->rate_min = 48000;
		break;
	case 0x258:
	case 0x260:
		fp->rate_max = fp->rate_min = 96000;
		break;
	default:
		usb_audio_err(chip, "unknown sample rate\n");
		kfree(fp);
		return -ENXIO;
	}

	stream = (fp->endpoint & USB_DIR_IN)
		? SNDRV_PCM_STREAM_CAPTURE : SNDRV_PCM_STREAM_PLAYBACK;
	err = snd_usb_add_audio_stream(chip, stream, fp);
	if (err < 0) {
		kfree(fp);
		return err;
	}
	usb_set_interface(chip->dev, fp->iface, 0);
	return 0;
}