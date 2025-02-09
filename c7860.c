void dwc3_gadget_giveback(struct dwc3_ep *dep, struct dwc3_request *req,
		int status)
{
	struct dwc3			*dwc = dep->dwc;

	req->started = false;
	list_del(&req->list);
	req->remaining = 0;

	if (req->request.status == -EINPROGRESS)
		req->request.status = status;

	if (req->trb)
		usb_gadget_unmap_request_by_dev(dwc->sysdev,
						&req->request, req->direction);

	req->trb = NULL;

	trace_dwc3_gadget_giveback(req);

	spin_unlock(&dwc->lock);
	usb_gadget_giveback_request(&dep->endpoint, &req->request);
	spin_lock(&dwc->lock);

	if (dep->number > 1)
		pm_runtime_put(dwc->dev);
}