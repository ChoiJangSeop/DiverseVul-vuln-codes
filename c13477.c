static void dma_rx(struct b43_dmaring *ring, int *slot)
{
	const struct b43_dma_ops *ops = ring->ops;
	struct b43_dmadesc_generic *desc;
	struct b43_dmadesc_meta *meta;
	struct b43_rxhdr_fw4 *rxhdr;
	struct sk_buff *skb;
	u16 len;
	int err;
	dma_addr_t dmaaddr;

	desc = ops->idx2desc(ring, *slot, &meta);

	sync_descbuffer_for_cpu(ring, meta->dmaaddr, ring->rx_buffersize);
	skb = meta->skb;

	rxhdr = (struct b43_rxhdr_fw4 *)skb->data;
	len = le16_to_cpu(rxhdr->frame_len);
	if (len == 0) {
		int i = 0;

		do {
			udelay(2);
			barrier();
			len = le16_to_cpu(rxhdr->frame_len);
		} while (len == 0 && i++ < 5);
		if (unlikely(len == 0)) {
			dmaaddr = meta->dmaaddr;
			goto drop_recycle_buffer;
		}
	}
	if (unlikely(b43_rx_buffer_is_poisoned(ring, skb))) {
		/* Something went wrong with the DMA.
		 * The device did not touch the buffer and did not overwrite the poison. */
		b43dbg(ring->dev->wl, "DMA RX: Dropping poisoned buffer.\n");
		dmaaddr = meta->dmaaddr;
		goto drop_recycle_buffer;
	}
	if (unlikely(len > ring->rx_buffersize)) {
		/* The data did not fit into one descriptor buffer
		 * and is split over multiple buffers.
		 * This should never happen, as we try to allocate buffers
		 * big enough. So simply ignore this packet.
		 */
		int cnt = 0;
		s32 tmp = len;

		while (1) {
			desc = ops->idx2desc(ring, *slot, &meta);
			/* recycle the descriptor buffer. */
			b43_poison_rx_buffer(ring, meta->skb);
			sync_descbuffer_for_device(ring, meta->dmaaddr,
						   ring->rx_buffersize);
			*slot = next_slot(ring, *slot);
			cnt++;
			tmp -= ring->rx_buffersize;
			if (tmp <= 0)
				break;
		}
		b43err(ring->dev->wl, "DMA RX buffer too small "
		       "(len: %u, buffer: %u, nr-dropped: %d)\n",
		       len, ring->rx_buffersize, cnt);
		goto drop;
	}

	dmaaddr = meta->dmaaddr;
	err = setup_rx_descbuffer(ring, desc, meta, GFP_ATOMIC);
	if (unlikely(err)) {
		b43dbg(ring->dev->wl, "DMA RX: setup_rx_descbuffer() failed\n");
		goto drop_recycle_buffer;
	}

	unmap_descbuffer(ring, dmaaddr, ring->rx_buffersize, 0);
	skb_put(skb, len + ring->frameoffset);
	skb_pull(skb, ring->frameoffset);

	b43_rx(ring->dev, skb, rxhdr);
drop:
	return;

drop_recycle_buffer:
	/* Poison and recycle the RX buffer. */
	b43_poison_rx_buffer(ring, skb);
	sync_descbuffer_for_device(ring, dmaaddr, ring->rx_buffersize);
}