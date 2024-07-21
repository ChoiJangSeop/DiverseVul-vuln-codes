static void xudc_getstatus(struct xusb_udc *udc)
{
	struct xusb_ep *ep0	= &udc->ep[0];
	struct xusb_req *req	= udc->req;
	struct xusb_ep *target_ep;
	u16 status = 0;
	u32 epcfgreg;
	int epnum;
	u32 halt;
	int ret;

	switch (udc->setup.bRequestType & USB_RECIP_MASK) {
	case USB_RECIP_DEVICE:
		/* Get device status */
		status = 1 << USB_DEVICE_SELF_POWERED;
		if (udc->remote_wkp)
			status |= (1 << USB_DEVICE_REMOTE_WAKEUP);
		break;
	case USB_RECIP_INTERFACE:
		break;
	case USB_RECIP_ENDPOINT:
		epnum = udc->setup.wIndex & USB_ENDPOINT_NUMBER_MASK;
		target_ep = &udc->ep[epnum];
		epcfgreg = udc->read_fn(udc->addr + target_ep->offset);
		halt = epcfgreg & XUSB_EP_CFG_STALL_MASK;
		if (udc->setup.wIndex & USB_DIR_IN) {
			if (!target_ep->is_in)
				goto stall;
		} else {
			if (target_ep->is_in)
				goto stall;
		}
		if (halt)
			status = 1 << USB_ENDPOINT_HALT;
		break;
	default:
		goto stall;
	}

	req->usb_req.length = 2;
	*(u16 *)req->usb_req.buf = cpu_to_le16(status);
	ret = __xudc_ep0_queue(ep0, req);
	if (ret == 0)
		return;
stall:
	dev_err(udc->dev, "Can't respond to getstatus request\n");
	xudc_ep0_stall(udc);
}