static void xhci_kick_epctx(XHCIEPContext *epctx, unsigned int streamid)
{
    XHCIState *xhci = epctx->xhci;
    XHCIStreamContext *stctx;
    XHCITransfer *xfer;
    XHCIRing *ring;
    USBEndpoint *ep = NULL;
    uint64_t mfindex;
    int length;
    int i;

    trace_usb_xhci_ep_kick(epctx->slotid, epctx->epid, streamid);

    /* If the device has been detached, but the guest has not noticed this
       yet the 2 above checks will succeed, but we must NOT continue */
    if (!xhci->slots[epctx->slotid - 1].uport ||
        !xhci->slots[epctx->slotid - 1].uport->dev ||
        !xhci->slots[epctx->slotid - 1].uport->dev->attached) {
        return;
    }

    if (epctx->retry) {
        XHCITransfer *xfer = epctx->retry;

        trace_usb_xhci_xfer_retry(xfer);
        assert(xfer->running_retry);
        if (xfer->timed_xfer) {
            /* time to kick the transfer? */
            mfindex = xhci_mfindex_get(xhci);
            xhci_check_intr_iso_kick(xhci, xfer, epctx, mfindex);
            if (xfer->running_retry) {
                return;
            }
            xfer->timed_xfer = 0;
            xfer->running_retry = 1;
        }
        if (xfer->iso_xfer) {
            /* retry iso transfer */
            if (xhci_setup_packet(xfer) < 0) {
                return;
            }
            usb_handle_packet(xfer->packet.ep->dev, &xfer->packet);
            assert(xfer->packet.status != USB_RET_NAK);
            xhci_try_complete_packet(xfer);
        } else {
            /* retry nak'ed transfer */
            if (xhci_setup_packet(xfer) < 0) {
                return;
            }
            usb_handle_packet(xfer->packet.ep->dev, &xfer->packet);
            if (xfer->packet.status == USB_RET_NAK) {
                return;
            }
            xhci_try_complete_packet(xfer);
        }
        assert(!xfer->running_retry);
        if (xfer->complete) {
            xhci_ep_free_xfer(epctx->retry);
        }
        epctx->retry = NULL;
    }

    if (epctx->state == EP_HALTED) {
        DPRINTF("xhci: ep halted, not running schedule\n");
        return;
    }


    if (epctx->nr_pstreams) {
        uint32_t err;
        stctx = xhci_find_stream(epctx, streamid, &err);
        if (stctx == NULL) {
            return;
        }
        ring = &stctx->ring;
        xhci_set_ep_state(xhci, epctx, stctx, EP_RUNNING);
    } else {
        ring = &epctx->ring;
        streamid = 0;
        xhci_set_ep_state(xhci, epctx, NULL, EP_RUNNING);
    }
    assert(ring->dequeue != 0);

    while (1) {
        length = xhci_ring_chain_length(xhci, ring);
        if (length <= 0) {
            break;
        }
        xfer = xhci_ep_alloc_xfer(epctx, length);
        if (xfer == NULL) {
            break;
        }

        for (i = 0; i < length; i++) {
            TRBType type;
            type = xhci_ring_fetch(xhci, ring, &xfer->trbs[i], NULL);
            assert(type);
        }
        xfer->streamid = streamid;

        if (epctx->epid == 1) {
            xhci_fire_ctl_transfer(xhci, xfer);
        } else {
            xhci_fire_transfer(xhci, xfer, epctx);
        }
        if (xfer->complete) {
            xhci_ep_free_xfer(xfer);
            xfer = NULL;
        }

        if (epctx->state == EP_HALTED) {
            break;
        }
        if (xfer != NULL && xfer->running_retry) {
            DPRINTF("xhci: xfer nacked, stopping schedule\n");
            epctx->retry = xfer;
            break;
        }
    }

    ep = xhci_epid_to_usbep(epctx);
    if (ep) {
        usb_device_flush_ep_queue(ep->dev, ep);
    }
}