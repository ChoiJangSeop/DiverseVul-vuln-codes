static int stub_send_ret_submit(struct stub_device *sdev)
{
	unsigned long flags;
	struct stub_priv *priv, *tmp;

	struct msghdr msg;
	size_t txsize;

	size_t total_size = 0;

	while ((priv = dequeue_from_priv_tx(sdev)) != NULL) {
		int ret;
		struct urb *urb = priv->urb;
		struct usbip_header pdu_header;
		struct usbip_iso_packet_descriptor *iso_buffer = NULL;
		struct kvec *iov = NULL;
		int iovnum = 0;

		txsize = 0;
		memset(&pdu_header, 0, sizeof(pdu_header));
		memset(&msg, 0, sizeof(msg));

		if (usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS)
			iovnum = 2 + urb->number_of_packets;
		else
			iovnum = 2;

		iov = kcalloc(iovnum, sizeof(struct kvec), GFP_KERNEL);

		if (!iov) {
			usbip_event_add(&sdev->ud, SDEV_EVENT_ERROR_MALLOC);
			return -1;
		}

		iovnum = 0;

		/* 1. setup usbip_header */
		setup_ret_submit_pdu(&pdu_header, urb);
		usbip_dbg_stub_tx("setup txdata seqnum: %d urb: %p\n",
				  pdu_header.base.seqnum, urb);
		usbip_header_correct_endian(&pdu_header, 1);

		iov[iovnum].iov_base = &pdu_header;
		iov[iovnum].iov_len  = sizeof(pdu_header);
		iovnum++;
		txsize += sizeof(pdu_header);

		/* 2. setup transfer buffer */
		if (usb_pipein(urb->pipe) &&
		    usb_pipetype(urb->pipe) != PIPE_ISOCHRONOUS &&
		    urb->actual_length > 0) {
			iov[iovnum].iov_base = urb->transfer_buffer;
			iov[iovnum].iov_len  = urb->actual_length;
			iovnum++;
			txsize += urb->actual_length;
		} else if (usb_pipein(urb->pipe) &&
			   usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS) {
			/*
			 * For isochronous packets: actual length is the sum of
			 * the actual length of the individual, packets, but as
			 * the packet offsets are not changed there will be
			 * padding between the packets. To optimally use the
			 * bandwidth the padding is not transmitted.
			 */

			int i;

			for (i = 0; i < urb->number_of_packets; i++) {
				iov[iovnum].iov_base = urb->transfer_buffer +
					urb->iso_frame_desc[i].offset;
				iov[iovnum].iov_len =
					urb->iso_frame_desc[i].actual_length;
				iovnum++;
				txsize += urb->iso_frame_desc[i].actual_length;
			}

			if (txsize != sizeof(pdu_header) + urb->actual_length) {
				dev_err(&sdev->udev->dev,
					"actual length of urb %d does not match iso packet sizes %zu\n",
					urb->actual_length,
					txsize-sizeof(pdu_header));
				kfree(iov);
				usbip_event_add(&sdev->ud,
						SDEV_EVENT_ERROR_TCP);
			   return -1;
			}
		}

		/* 3. setup iso_packet_descriptor */
		if (usb_pipetype(urb->pipe) == PIPE_ISOCHRONOUS) {
			ssize_t len = 0;

			iso_buffer = usbip_alloc_iso_desc_pdu(urb, &len);
			if (!iso_buffer) {
				usbip_event_add(&sdev->ud,
						SDEV_EVENT_ERROR_MALLOC);
				kfree(iov);
				return -1;
			}

			iov[iovnum].iov_base = iso_buffer;
			iov[iovnum].iov_len  = len;
			txsize += len;
			iovnum++;
		}

		ret = kernel_sendmsg(sdev->ud.tcp_socket, &msg,
						iov,  iovnum, txsize);
		if (ret != txsize) {
			dev_err(&sdev->udev->dev,
				"sendmsg failed!, retval %d for %zd\n",
				ret, txsize);
			kfree(iov);
			kfree(iso_buffer);
			usbip_event_add(&sdev->ud, SDEV_EVENT_ERROR_TCP);
			return -1;
		}

		kfree(iov);
		kfree(iso_buffer);

		total_size += txsize;
	}

	spin_lock_irqsave(&sdev->priv_lock, flags);
	list_for_each_entry_safe(priv, tmp, &sdev->priv_free, list) {
		stub_free_priv_and_urb(priv);
	}
	spin_unlock_irqrestore(&sdev->priv_lock, flags);

	return total_size;
}