static bool gfar_add_rx_frag(struct gfar_rx_buff *rxb, u32 lstatus,
			     struct sk_buff *skb, bool first)
{
	int size = lstatus & BD_LENGTH_MASK;
	struct page *page = rxb->page;

	if (likely(first)) {
		skb_put(skb, size);
	} else {
		/* the last fragments' length contains the full frame length */
		if (lstatus & BD_LFLAG(RXBD_LAST))
			size -= skb->len;

		skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, page,
				rxb->page_offset + RXBUF_ALIGNMENT,
				size, GFAR_RXB_TRUESIZE);
	}

	/* try reuse page */
	if (unlikely(page_count(page) != 1 || page_is_pfmemalloc(page)))
		return false;

	/* change offset to the other half */
	rxb->page_offset ^= GFAR_RXB_TRUESIZE;

	page_ref_inc(page);

	return true;
}