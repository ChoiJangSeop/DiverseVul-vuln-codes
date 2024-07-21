dma_async_memcpy_pg_to_pg(struct dma_chan *chan, struct page *dest_pg,
	unsigned int dest_off, struct page *src_pg, unsigned int src_off,
	size_t len)
{
	struct dma_device *dev = chan->device;
	struct dma_async_tx_descriptor *tx;
	struct dmaengine_unmap_data *unmap;
	dma_cookie_t cookie;
	unsigned long flags;

	unmap = dmaengine_get_unmap_data(dev->dev, 2, GFP_NOWAIT);
	if (!unmap)
		return -ENOMEM;

	unmap->to_cnt = 1;
	unmap->from_cnt = 1;
	unmap->addr[0] = dma_map_page(dev->dev, src_pg, src_off, len,
				      DMA_TO_DEVICE);
	unmap->addr[1] = dma_map_page(dev->dev, dest_pg, dest_off, len,
				      DMA_FROM_DEVICE);
	unmap->len = len;
	flags = DMA_CTRL_ACK;
	tx = dev->device_prep_dma_memcpy(chan, unmap->addr[1], unmap->addr[0],
					 len, flags);

	if (!tx) {
		dmaengine_unmap_put(unmap);
		return -ENOMEM;
	}

	dma_set_unmap(tx, unmap);
	cookie = tx->tx_submit(tx);
	dmaengine_unmap_put(unmap);

	preempt_disable();
	__this_cpu_add(chan->local->bytes_transferred, len);
	__this_cpu_inc(chan->local->memcpy_count);
	preempt_enable();

	return cookie;
}