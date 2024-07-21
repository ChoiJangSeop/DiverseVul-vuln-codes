static int snd_microii_spdif_default_get(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct usb_mixer_elem_list *list = snd_kcontrol_chip(kcontrol);
	struct snd_usb_audio *chip = list->mixer->chip;
	int err;
	struct usb_interface *iface;
	struct usb_host_interface *alts;
	unsigned int ep;
	unsigned char data[3];
	int rate;

	err = snd_usb_lock_shutdown(chip);
	if (err < 0)
		return err;

	ucontrol->value.iec958.status[0] = kcontrol->private_value & 0xff;
	ucontrol->value.iec958.status[1] = (kcontrol->private_value >> 8) & 0xff;
	ucontrol->value.iec958.status[2] = 0x00;

	/* use known values for that card: interface#1 altsetting#1 */
	iface = usb_ifnum_to_if(chip->dev, 1);
	alts = &iface->altsetting[1];
	ep = get_endpoint(alts, 0)->bEndpointAddress;

	err = snd_usb_ctl_msg(chip->dev,
			usb_rcvctrlpipe(chip->dev, 0),
			UAC_GET_CUR,
			USB_TYPE_CLASS | USB_RECIP_ENDPOINT | USB_DIR_IN,
			UAC_EP_CS_ATTR_SAMPLE_RATE << 8,
			ep,
			data,
			sizeof(data));
	if (err < 0)
		goto end;

	rate = data[0] | (data[1] << 8) | (data[2] << 16);
	ucontrol->value.iec958.status[3] = (rate == 48000) ?
			IEC958_AES3_CON_FS_48000 : IEC958_AES3_CON_FS_44100;

	err = 0;
 end:
	snd_usb_unlock_shutdown(chip);
	return err;
}