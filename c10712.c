max3421_urb_done(struct usb_hcd *hcd)
{
	struct max3421_hcd *max3421_hcd = hcd_to_max3421(hcd);
	unsigned long flags;
	struct urb *urb;
	int status;

	status = max3421_hcd->urb_done;
	max3421_hcd->urb_done = 0;
	if (status > 0)
		status = 0;
	urb = max3421_hcd->curr_urb;
	if (urb) {
		max3421_hcd->curr_urb = NULL;
		spin_lock_irqsave(&max3421_hcd->lock, flags);
		usb_hcd_unlink_urb_from_ep(hcd, urb);
		spin_unlock_irqrestore(&max3421_hcd->lock, flags);

		/* must be called without the HCD spinlock: */
		usb_hcd_giveback_urb(hcd, urb, status);
	}
	return 1;
}