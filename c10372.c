static void usbredir_handle_iso_data(USBRedirDevice *dev, USBPacket *p,
                                     uint8_t ep)
{
    int status, len;
    if (!dev->endpoint[EP2I(ep)].iso_started &&
            !dev->endpoint[EP2I(ep)].iso_error) {
        struct usb_redir_start_iso_stream_header start_iso = {
            .endpoint = ep,
        };
        int pkts_per_sec;

        if (dev->dev.speed == USB_SPEED_HIGH) {
            pkts_per_sec = 8000 / dev->endpoint[EP2I(ep)].interval;
        } else {
            pkts_per_sec = 1000 / dev->endpoint[EP2I(ep)].interval;
        }
        /* Testing has shown that we need circa 60 ms buffer */
        dev->endpoint[EP2I(ep)].bufpq_target_size = (pkts_per_sec * 60) / 1000;

        /* Aim for approx 100 interrupts / second on the client to
           balance latency and interrupt load */
        start_iso.pkts_per_urb = pkts_per_sec / 100;
        if (start_iso.pkts_per_urb < 1) {
            start_iso.pkts_per_urb = 1;
        } else if (start_iso.pkts_per_urb > 32) {
            start_iso.pkts_per_urb = 32;
        }

        start_iso.no_urbs = DIV_ROUND_UP(
                                     dev->endpoint[EP2I(ep)].bufpq_target_size,
                                     start_iso.pkts_per_urb);
        /* Output endpoints pre-fill only 1/2 of the packets, keeping the rest
           as overflow buffer. Also see the usbredir protocol documentation */
        if (!(ep & USB_DIR_IN)) {
            start_iso.no_urbs *= 2;
        }
        if (start_iso.no_urbs > 16) {
            start_iso.no_urbs = 16;
        }

        /* No id, we look at the ep when receiving a status back */
        usbredirparser_send_start_iso_stream(dev->parser, 0, &start_iso);
        usbredirparser_do_write(dev->parser);
        DPRINTF("iso stream started pkts/sec %d pkts/urb %d urbs %d ep %02X\n",
                pkts_per_sec, start_iso.pkts_per_urb, start_iso.no_urbs, ep);
        dev->endpoint[EP2I(ep)].iso_started = 1;
        dev->endpoint[EP2I(ep)].bufpq_prefilled = 0;
        dev->endpoint[EP2I(ep)].bufpq_dropping_packets = 0;
    }

    if (ep & USB_DIR_IN) {
        struct buf_packet *isop;

        if (dev->endpoint[EP2I(ep)].iso_started &&
                !dev->endpoint[EP2I(ep)].bufpq_prefilled) {
            if (dev->endpoint[EP2I(ep)].bufpq_size <
                    dev->endpoint[EP2I(ep)].bufpq_target_size) {
                return;
            }
            dev->endpoint[EP2I(ep)].bufpq_prefilled = 1;
        }

        isop = QTAILQ_FIRST(&dev->endpoint[EP2I(ep)].bufpq);
        if (isop == NULL) {
            DPRINTF("iso-token-in ep %02X, no isop, iso_error: %d\n",
                    ep, dev->endpoint[EP2I(ep)].iso_error);
            /* Re-fill the buffer */
            dev->endpoint[EP2I(ep)].bufpq_prefilled = 0;
            /* Check iso_error for stream errors, otherwise its an underrun */
            status = dev->endpoint[EP2I(ep)].iso_error;
            dev->endpoint[EP2I(ep)].iso_error = 0;
            p->status = status ? USB_RET_IOERROR : USB_RET_SUCCESS;
            return;
        }
        DPRINTF2("iso-token-in ep %02X status %d len %d queue-size: %d\n", ep,
                 isop->status, isop->len, dev->endpoint[EP2I(ep)].bufpq_size);

        status = isop->status;
        len = isop->len;
        if (len > p->iov.size) {
            ERROR("received iso data is larger then packet ep %02X (%d > %d)\n",
                  ep, len, (int)p->iov.size);
            len = p->iov.size;
            status = usb_redir_babble;
        }
        usb_packet_copy(p, isop->data, len);
        bufp_free(dev, isop, ep);
        usbredir_handle_status(dev, p, status);
    } else {
        /* If the stream was not started because of a pending error don't
           send the packet to the usb-host */
        if (dev->endpoint[EP2I(ep)].iso_started) {
            struct usb_redir_iso_packet_header iso_packet = {
                .endpoint = ep,
                .length = p->iov.size
            };
            uint8_t buf[p->iov.size];
            /* No id, we look at the ep when receiving a status back */
            usb_packet_copy(p, buf, p->iov.size);
            usbredirparser_send_iso_packet(dev->parser, 0, &iso_packet,
                                           buf, p->iov.size);
            usbredirparser_do_write(dev->parser);
        }
        status = dev->endpoint[EP2I(ep)].iso_error;
        dev->endpoint[EP2I(ep)].iso_error = 0;
        DPRINTF2("iso-token-out ep %02X status %d len %zd\n", ep, status,
                 p->iov.size);
        usbredir_handle_status(dev, p, status);
    }
}