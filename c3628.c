
static int ims_pcu_parse_cdc_data(struct usb_interface *intf, struct ims_pcu *pcu)
{
	const struct usb_cdc_union_desc *union_desc;
	struct usb_host_interface *alt;

	union_desc = ims_pcu_get_cdc_union_desc(intf);
	if (!union_desc)
		return -EINVAL;

	pcu->ctrl_intf = usb_ifnum_to_if(pcu->udev,
					 union_desc->bMasterInterface0);

	alt = pcu->ctrl_intf->cur_altsetting;
	pcu->ep_ctrl = &alt->endpoint[0].desc;
	pcu->max_ctrl_size = usb_endpoint_maxp(pcu->ep_ctrl);

	pcu->data_intf = usb_ifnum_to_if(pcu->udev,
					 union_desc->bSlaveInterface0);

	alt = pcu->data_intf->cur_altsetting;
	if (alt->desc.bNumEndpoints != 2) {
		dev_err(pcu->dev,
			"Incorrect number of endpoints on data interface (%d)\n",
			alt->desc.bNumEndpoints);
		return -EINVAL;
	}

	pcu->ep_out = &alt->endpoint[0].desc;
	if (!usb_endpoint_is_bulk_out(pcu->ep_out)) {
		dev_err(pcu->dev,
			"First endpoint on data interface is not BULK OUT\n");
		return -EINVAL;
	}

	pcu->max_out_size = usb_endpoint_maxp(pcu->ep_out);
	if (pcu->max_out_size < 8) {
		dev_err(pcu->dev,
			"Max OUT packet size is too small (%zd)\n",
			pcu->max_out_size);
		return -EINVAL;
	}

	pcu->ep_in = &alt->endpoint[1].desc;
	if (!usb_endpoint_is_bulk_in(pcu->ep_in)) {
		dev_err(pcu->dev,
			"Second endpoint on data interface is not BULK IN\n");
		return -EINVAL;
	}

	pcu->max_in_size = usb_endpoint_maxp(pcu->ep_in);
	if (pcu->max_in_size < 8) {
		dev_err(pcu->dev,
			"Max IN packet size is too small (%zd)\n",
			pcu->max_in_size);
		return -EINVAL;
	}

	return 0;