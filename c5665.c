static int uas_find_uas_alt_setting(struct usb_interface *intf)
{
	int i;

	for (i = 0; i < intf->num_altsetting; i++) {
		struct usb_host_interface *alt = &intf->altsetting[i];

		if (uas_is_interface(alt))
			return alt->desc.bAlternateSetting;
	}

	return -ENODEV;
}