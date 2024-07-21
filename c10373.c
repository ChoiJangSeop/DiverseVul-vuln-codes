static void usbredir_handle_bulk_data(USBRedirDevice *dev, USBPacket *p,
                                      uint8_t ep)
{
    struct usb_redir_bulk_packet_header bulk_packet;
    size_t size = usb_packet_size(p);
    const int maxp = dev->endpoint[EP2I(ep)].max_packet_size;

    if (usbredir_already_in_flight(dev, p->id)) {
        p->status = USB_RET_ASYNC;
        return;
    }

    if (dev->endpoint[EP2I(ep)].bulk_receiving_enabled) {
        if (size != 0 && (size % maxp) == 0) {
            usbredir_handle_buffered_bulk_in_data(dev, p, ep);
            return;
        }
        WARNING("bulk recv invalid size %zd ep %02x, disabling\n", size, ep);
        assert(dev->endpoint[EP2I(ep)].pending_async_packet == NULL);
        usbredir_stop_bulk_receiving(dev, ep);
        dev->endpoint[EP2I(ep)].bulk_receiving_enabled = 0;
    }

    DPRINTF("bulk-out ep %02X stream %u len %zd id %"PRIu64"\n",
            ep, p->stream, size, p->id);

    bulk_packet.endpoint  = ep;
    bulk_packet.length    = size;
    bulk_packet.stream_id = p->stream;
    bulk_packet.length_high = size >> 16;
    assert(bulk_packet.length_high == 0 ||
           usbredirparser_peer_has_cap(dev->parser,
                                       usb_redir_cap_32bits_bulk_length));

    if (ep & USB_DIR_IN || size == 0) {
        usbredirparser_send_bulk_packet(dev->parser, p->id,
                                        &bulk_packet, NULL, 0);
    } else {
        uint8_t buf[size];
        usb_packet_copy(p, buf, size);
        usbredir_log_data(dev, "bulk data out:", buf, size);
        usbredirparser_send_bulk_packet(dev->parser, p->id,
                                        &bulk_packet, buf, size);
    }
    usbredirparser_do_write(dev->parser);
    p->status = USB_RET_ASYNC;
}