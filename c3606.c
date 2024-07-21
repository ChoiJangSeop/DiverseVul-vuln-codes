int snd_usb_parse_audio_interface(struct snd_usb_audio *chip, int iface_no)
{
	struct usb_device *dev;
	struct usb_interface *iface;
	struct usb_host_interface *alts;
	struct usb_interface_descriptor *altsd;
	int i, altno, err, stream;
	unsigned int format = 0, num_channels = 0;
	struct audioformat *fp = NULL;
	int num, protocol, clock = 0;
	struct uac_format_type_i_continuous_descriptor *fmt;
	unsigned int chconfig;

	dev = chip->dev;

	/* parse the interface's altsettings */
	iface = usb_ifnum_to_if(dev, iface_no);

	num = iface->num_altsetting;

	/*
	 * Dallas DS4201 workaround: It presents 5 altsettings, but the last
	 * one misses syncpipe, and does not produce any sound.
	 */
	if (chip->usb_id == USB_ID(0x04fa, 0x4201))
		num = 4;

	for (i = 0; i < num; i++) {
		alts = &iface->altsetting[i];
		altsd = get_iface_desc(alts);
		protocol = altsd->bInterfaceProtocol;
		/* skip invalid one */
		if (((altsd->bInterfaceClass != USB_CLASS_AUDIO ||
		      (altsd->bInterfaceSubClass != USB_SUBCLASS_AUDIOSTREAMING &&
		       altsd->bInterfaceSubClass != USB_SUBCLASS_VENDOR_SPEC)) &&
		     altsd->bInterfaceClass != USB_CLASS_VENDOR_SPEC) ||
		    altsd->bNumEndpoints < 1 ||
		    le16_to_cpu(get_endpoint(alts, 0)->wMaxPacketSize) == 0)
			continue;
		/* must be isochronous */
		if ((get_endpoint(alts, 0)->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) !=
		    USB_ENDPOINT_XFER_ISOC)
			continue;
		/* check direction */
		stream = (get_endpoint(alts, 0)->bEndpointAddress & USB_DIR_IN) ?
			SNDRV_PCM_STREAM_CAPTURE : SNDRV_PCM_STREAM_PLAYBACK;
		altno = altsd->bAlternateSetting;

		if (snd_usb_apply_interface_quirk(chip, iface_no, altno))
			continue;

		/*
		 * Roland audio streaming interfaces are marked with protocols
		 * 0/1/2, but are UAC 1 compatible.
		 */
		if (USB_ID_VENDOR(chip->usb_id) == 0x0582 &&
		    altsd->bInterfaceClass == USB_CLASS_VENDOR_SPEC &&
		    protocol <= 2)
			protocol = UAC_VERSION_1;

		chconfig = 0;
		/* get audio formats */
		switch (protocol) {
		default:
			dev_dbg(&dev->dev, "%u:%d: unknown interface protocol %#02x, assuming v1\n",
				iface_no, altno, protocol);
			protocol = UAC_VERSION_1;
			/* fall through */

		case UAC_VERSION_1: {
			struct uac1_as_header_descriptor *as =
				snd_usb_find_csint_desc(alts->extra, alts->extralen, NULL, UAC_AS_GENERAL);
			struct uac_input_terminal_descriptor *iterm;

			if (!as) {
				dev_err(&dev->dev,
					"%u:%d : UAC_AS_GENERAL descriptor not found\n",
					iface_no, altno);
				continue;
			}

			if (as->bLength < sizeof(*as)) {
				dev_err(&dev->dev,
					"%u:%d : invalid UAC_AS_GENERAL desc\n",
					iface_no, altno);
				continue;
			}

			format = le16_to_cpu(as->wFormatTag); /* remember the format value */

			iterm = snd_usb_find_input_terminal_descriptor(chip->ctrl_intf,
								       as->bTerminalLink);
			if (iterm) {
				num_channels = iterm->bNrChannels;
				chconfig = le16_to_cpu(iterm->wChannelConfig);
			}

			break;
		}

		case UAC_VERSION_2: {
			struct uac2_input_terminal_descriptor *input_term;
			struct uac2_output_terminal_descriptor *output_term;
			struct uac2_as_header_descriptor *as =
				snd_usb_find_csint_desc(alts->extra, alts->extralen, NULL, UAC_AS_GENERAL);

			if (!as) {
				dev_err(&dev->dev,
					"%u:%d : UAC_AS_GENERAL descriptor not found\n",
					iface_no, altno);
				continue;
			}

			if (as->bLength < sizeof(*as)) {
				dev_err(&dev->dev,
					"%u:%d : invalid UAC_AS_GENERAL desc\n",
					iface_no, altno);
				continue;
			}

			num_channels = as->bNrChannels;
			format = le32_to_cpu(as->bmFormats);
			chconfig = le32_to_cpu(as->bmChannelConfig);

			/* lookup the terminal associated to this interface
			 * to extract the clock */
			input_term = snd_usb_find_input_terminal_descriptor(chip->ctrl_intf,
									    as->bTerminalLink);
			if (input_term) {
				clock = input_term->bCSourceID;
				if (!chconfig && (num_channels == input_term->bNrChannels))
					chconfig = le32_to_cpu(input_term->bmChannelConfig);
				break;
			}

			output_term = snd_usb_find_output_terminal_descriptor(chip->ctrl_intf,
									      as->bTerminalLink);
			if (output_term) {
				clock = output_term->bCSourceID;
				break;
			}

			dev_err(&dev->dev,
				"%u:%d : bogus bTerminalLink %d\n",
				iface_no, altno, as->bTerminalLink);
			continue;
		}
		}

		/* get format type */
		fmt = snd_usb_find_csint_desc(alts->extra, alts->extralen, NULL, UAC_FORMAT_TYPE);
		if (!fmt) {
			dev_err(&dev->dev,
				"%u:%d : no UAC_FORMAT_TYPE desc\n",
				iface_no, altno);
			continue;
		}
		if (((protocol == UAC_VERSION_1) && (fmt->bLength < 8)) ||
		    ((protocol == UAC_VERSION_2) && (fmt->bLength < 6))) {
			dev_err(&dev->dev,
				"%u:%d : invalid UAC_FORMAT_TYPE desc\n",
				iface_no, altno);
			continue;
		}

		/*
		 * Blue Microphones workaround: The last altsetting is identical
		 * with the previous one, except for a larger packet size, but
		 * is actually a mislabeled two-channel setting; ignore it.
		 */
		if (fmt->bNrChannels == 1 &&
		    fmt->bSubframeSize == 2 &&
		    altno == 2 && num == 3 &&
		    fp && fp->altsetting == 1 && fp->channels == 1 &&
		    fp->formats == SNDRV_PCM_FMTBIT_S16_LE &&
		    protocol == UAC_VERSION_1 &&
		    le16_to_cpu(get_endpoint(alts, 0)->wMaxPacketSize) ==
							fp->maxpacksize * 2)
			continue;

		fp = kzalloc(sizeof(*fp), GFP_KERNEL);
		if (! fp) {
			dev_err(&dev->dev, "cannot malloc\n");
			return -ENOMEM;
		}

		fp->iface = iface_no;
		fp->altsetting = altno;
		fp->altset_idx = i;
		fp->endpoint = get_endpoint(alts, 0)->bEndpointAddress;
		fp->ep_attr = get_endpoint(alts, 0)->bmAttributes;
		fp->datainterval = snd_usb_parse_datainterval(chip, alts);
		fp->protocol = protocol;
		fp->maxpacksize = le16_to_cpu(get_endpoint(alts, 0)->wMaxPacketSize);
		fp->channels = num_channels;
		if (snd_usb_get_speed(dev) == USB_SPEED_HIGH)
			fp->maxpacksize = (((fp->maxpacksize >> 11) & 3) + 1)
					* (fp->maxpacksize & 0x7ff);
		fp->attributes = parse_uac_endpoint_attributes(chip, alts, protocol, iface_no);
		fp->clock = clock;

		/* some quirks for attributes here */

		switch (chip->usb_id) {
		case USB_ID(0x0a92, 0x0053): /* AudioTrak Optoplay */
			/* Optoplay sets the sample rate attribute although
			 * it seems not supporting it in fact.
			 */
			fp->attributes &= ~UAC_EP_CS_ATTR_SAMPLE_RATE;
			break;
		case USB_ID(0x041e, 0x3020): /* Creative SB Audigy 2 NX */
		case USB_ID(0x0763, 0x2003): /* M-Audio Audiophile USB */
			/* doesn't set the sample rate attribute, but supports it */
			fp->attributes |= UAC_EP_CS_ATTR_SAMPLE_RATE;
			break;
		case USB_ID(0x0763, 0x2001):  /* M-Audio Quattro USB */
		case USB_ID(0x0763, 0x2012):  /* M-Audio Fast Track Pro USB */
		case USB_ID(0x047f, 0x0ca1): /* plantronics headset */
		case USB_ID(0x077d, 0x07af): /* Griffin iMic (note that there is
						an older model 77d:223) */
		/*
		 * plantronics headset and Griffin iMic have set adaptive-in
		 * although it's really not...
		 */
			fp->ep_attr &= ~USB_ENDPOINT_SYNCTYPE;
			if (stream == SNDRV_PCM_STREAM_PLAYBACK)
				fp->ep_attr |= USB_ENDPOINT_SYNC_ADAPTIVE;
			else
				fp->ep_attr |= USB_ENDPOINT_SYNC_SYNC;
			break;
		}

		/* ok, let's parse further... */
		if (snd_usb_parse_audio_format(chip, fp, format, fmt, stream) < 0) {
			kfree(fp->rate_table);
			kfree(fp);
			fp = NULL;
			continue;
		}

		/* Create chmap */
		if (fp->channels != num_channels)
			chconfig = 0;
		fp->chmap = convert_chmap(fp->channels, chconfig, protocol);

		dev_dbg(&dev->dev, "%u:%d: add audio endpoint %#x\n", iface_no, altno, fp->endpoint);
		err = snd_usb_add_audio_stream(chip, stream, fp);
		if (err < 0) {
			kfree(fp->rate_table);
			kfree(fp->chmap);
			kfree(fp);
			return err;
		}
		/* try to set the interface... */
		usb_set_interface(chip->dev, iface_no, altno);
		snd_usb_init_pitch(chip, iface_no, alts, fp);
		snd_usb_init_sample_rate(chip, iface_no, alts, fp, fp->rate_max);
	}
	return 0;
}