static void fwnet_receive_broadcast(struct fw_iso_context *context,
		u32 cycle, size_t header_length, void *header, void *data)
{
	struct fwnet_device *dev;
	struct fw_iso_packet packet;
	__be16 *hdr_ptr;
	__be32 *buf_ptr;
	int retval;
	u32 length;
	u16 source_node_id;
	u32 specifier_id;
	u32 ver;
	unsigned long offset;
	unsigned long flags;

	dev = data;
	hdr_ptr = header;
	length = be16_to_cpup(hdr_ptr);

	spin_lock_irqsave(&dev->lock, flags);

	offset = dev->rcv_buffer_size * dev->broadcast_rcv_next_ptr;
	buf_ptr = dev->broadcast_rcv_buffer_ptrs[dev->broadcast_rcv_next_ptr++];
	if (dev->broadcast_rcv_next_ptr == dev->num_broadcast_rcv_ptrs)
		dev->broadcast_rcv_next_ptr = 0;

	spin_unlock_irqrestore(&dev->lock, flags);

	specifier_id =    (be32_to_cpu(buf_ptr[0]) & 0xffff) << 8
			| (be32_to_cpu(buf_ptr[1]) & 0xff000000) >> 24;
	ver = be32_to_cpu(buf_ptr[1]) & 0xffffff;
	source_node_id = be32_to_cpu(buf_ptr[0]) >> 16;

	if (specifier_id == IANA_SPECIFIER_ID &&
	    (ver == RFC2734_SW_VERSION
#if IS_ENABLED(CONFIG_IPV6)
	     || ver == RFC3146_SW_VERSION
#endif
	    )) {
		buf_ptr += 2;
		length -= IEEE1394_GASP_HDR_SIZE;
		fwnet_incoming_packet(dev, buf_ptr, length, source_node_id,
				      context->card->generation, true);
	}

	packet.payload_length = dev->rcv_buffer_size;
	packet.interrupt = 1;
	packet.skip = 0;
	packet.tag = 3;
	packet.sy = 0;
	packet.header_length = IEEE1394_GASP_HDR_SIZE;

	spin_lock_irqsave(&dev->lock, flags);

	retval = fw_iso_context_queue(dev->broadcast_rcv_context, &packet,
				      &dev->broadcast_rcv_buffer, offset);

	spin_unlock_irqrestore(&dev->lock, flags);

	if (retval >= 0)
		fw_iso_context_queue_flush(dev->broadcast_rcv_context);
	else
		dev_err(&dev->netdev->dev, "requeue failed\n");
}