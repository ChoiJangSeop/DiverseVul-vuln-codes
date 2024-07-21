static int p54u_probe(struct usb_interface *intf,
				const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct ieee80211_hw *dev;
	struct p54u_priv *priv;
	int err;
	unsigned int i, recognized_pipes;

	dev = p54_init_common(sizeof(*priv));

	if (!dev) {
		dev_err(&udev->dev, "(p54usb) ieee80211 alloc failed\n");
		return -ENOMEM;
	}

	priv = dev->priv;
	priv->hw_type = P54U_INVALID_HW;

	SET_IEEE80211_DEV(dev, &intf->dev);
	usb_set_intfdata(intf, dev);
	priv->udev = udev;
	priv->intf = intf;
	skb_queue_head_init(&priv->rx_queue);
	init_usb_anchor(&priv->submitted);

	usb_get_dev(udev);

	/* really lazy and simple way of figuring out if we're a 3887 */
	/* TODO: should just stick the identification in the device table */
	i = intf->altsetting->desc.bNumEndpoints;
	recognized_pipes = 0;
	while (i--) {
		switch (intf->altsetting->endpoint[i].desc.bEndpointAddress) {
		case P54U_PIPE_DATA:
		case P54U_PIPE_MGMT:
		case P54U_PIPE_BRG:
		case P54U_PIPE_DEV:
		case P54U_PIPE_DATA | USB_DIR_IN:
		case P54U_PIPE_MGMT | USB_DIR_IN:
		case P54U_PIPE_BRG | USB_DIR_IN:
		case P54U_PIPE_DEV | USB_DIR_IN:
		case P54U_PIPE_INT | USB_DIR_IN:
			recognized_pipes++;
		}
	}
	priv->common.open = p54u_open;
	priv->common.stop = p54u_stop;
	if (recognized_pipes < P54U_PIPE_NUMBER) {
#ifdef CONFIG_PM
		/* ISL3887 needs a full reset on resume */
		udev->reset_resume = 1;
#endif /* CONFIG_PM */
		err = p54u_device_reset(dev);

		priv->hw_type = P54U_3887;
		dev->extra_tx_headroom += sizeof(struct lm87_tx_hdr);
		priv->common.tx_hdr_len = sizeof(struct lm87_tx_hdr);
		priv->common.tx = p54u_tx_lm87;
		priv->upload_fw = p54u_upload_firmware_3887;
	} else {
		priv->hw_type = P54U_NET2280;
		dev->extra_tx_headroom += sizeof(struct net2280_tx_hdr);
		priv->common.tx_hdr_len = sizeof(struct net2280_tx_hdr);
		priv->common.tx = p54u_tx_net2280;
		priv->upload_fw = p54u_upload_firmware_net2280;
	}
	err = p54u_load_firmware(dev, intf);
	if (err) {
		usb_put_dev(udev);
		p54_free_common(dev);
	}
	return err;
}