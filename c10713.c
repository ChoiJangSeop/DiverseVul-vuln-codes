max3421_set_address(struct usb_hcd *hcd, struct usb_device *dev, int epnum,
		    int force_toggles)
{
	struct max3421_hcd *max3421_hcd = hcd_to_max3421(hcd);
	int old_epnum, same_ep, rcvtog, sndtog;
	struct usb_device *old_dev;
	u8 hctl;

	old_dev = max3421_hcd->loaded_dev;
	old_epnum = max3421_hcd->loaded_epnum;

	same_ep = (dev == old_dev && epnum == old_epnum);
	if (same_ep && !force_toggles)
		return;

	if (old_dev && !same_ep) {
		/* save the old end-points toggles: */
		u8 hrsl = spi_rd8(hcd, MAX3421_REG_HRSL);

		rcvtog = (hrsl >> MAX3421_HRSL_RCVTOGRD_BIT) & 1;
		sndtog = (hrsl >> MAX3421_HRSL_SNDTOGRD_BIT) & 1;

		/* no locking: HCD (i.e., we) own toggles, don't we? */
		usb_settoggle(old_dev, old_epnum, 0, rcvtog);
		usb_settoggle(old_dev, old_epnum, 1, sndtog);
	}
	/* setup new endpoint's toggle bits: */
	rcvtog = usb_gettoggle(dev, epnum, 0);
	sndtog = usb_gettoggle(dev, epnum, 1);
	hctl = (BIT(rcvtog + MAX3421_HCTL_RCVTOG0_BIT) |
		BIT(sndtog + MAX3421_HCTL_SNDTOG0_BIT));

	max3421_hcd->loaded_epnum = epnum;
	spi_wr8(hcd, MAX3421_REG_HCTL, hctl);

	/*
	 * Note: devnum for one and the same device can change during
	 * address-assignment so it's best to just always load the
	 * address whenever the end-point changed/was forced.
	 */
	max3421_hcd->loaded_dev = dev;
	spi_wr8(hcd, MAX3421_REG_PERADDR, dev->devnum);
}