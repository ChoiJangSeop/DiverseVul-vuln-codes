static int init_pitch_v1(struct snd_usb_audio *chip, int iface,
			 struct usb_host_interface *alts,
			 struct audioformat *fmt)
{
	struct usb_device *dev = chip->dev;
	unsigned int ep;
	unsigned char data[1];
	int err;

	ep = get_endpoint(alts, 0)->bEndpointAddress;

	data[0] = 1;
	if ((err = snd_usb_ctl_msg(dev, usb_sndctrlpipe(dev, 0), UAC_SET_CUR,
				   USB_TYPE_CLASS|USB_RECIP_ENDPOINT|USB_DIR_OUT,
				   UAC_EP_CS_ATTR_PITCH_CONTROL << 8, ep,
				   data, sizeof(data))) < 0) {
		usb_audio_err(chip, "%d:%d: cannot set enable PITCH\n",
			      iface, ep);
		return err;
	}

	return 0;
}