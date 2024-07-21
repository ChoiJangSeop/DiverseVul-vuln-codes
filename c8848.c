vhost_reserve_avail_batch_packed(struct virtio_net *dev,
				 struct vhost_virtqueue *vq,
				 struct rte_mempool *mbuf_pool,
				 struct rte_mbuf **pkts,
				 uint16_t avail_idx,
				 uintptr_t *desc_addrs,
				 uint16_t *ids)
{
	bool wrap = vq->avail_wrap_counter;
	struct vring_packed_desc *descs = vq->desc_packed;
	struct virtio_net_hdr *hdr;
	uint64_t lens[PACKED_BATCH_SIZE];
	uint64_t buf_lens[PACKED_BATCH_SIZE];
	uint32_t buf_offset = dev->vhost_hlen;
	uint16_t flags, i;

	if (unlikely(avail_idx & PACKED_BATCH_MASK))
		return -1;
	if (unlikely((avail_idx + PACKED_BATCH_SIZE) > vq->size))
		return -1;

	vhost_for_each_try_unroll(i, 0, PACKED_BATCH_SIZE) {
		flags = descs[avail_idx + i].flags;
		if (unlikely((wrap != !!(flags & VRING_DESC_F_AVAIL)) ||
			     (wrap == !!(flags & VRING_DESC_F_USED))  ||
			     (flags & PACKED_DESC_SINGLE_DEQUEUE_FLAG)))
			return -1;
	}

	rte_smp_rmb();

	vhost_for_each_try_unroll(i, 0, PACKED_BATCH_SIZE)
		lens[i] = descs[avail_idx + i].len;

	vhost_for_each_try_unroll(i, 0, PACKED_BATCH_SIZE) {
		desc_addrs[i] = vhost_iova_to_vva(dev, vq,
						  descs[avail_idx + i].addr,
						  &lens[i], VHOST_ACCESS_RW);
	}

	vhost_for_each_try_unroll(i, 0, PACKED_BATCH_SIZE) {
		if (unlikely((lens[i] != descs[avail_idx + i].len)))
			return -1;
	}

	vhost_for_each_try_unroll(i, 0, PACKED_BATCH_SIZE) {
		pkts[i] = virtio_dev_pktmbuf_alloc(dev, mbuf_pool, lens[i]);
		if (!pkts[i])
			goto free_buf;
	}

	vhost_for_each_try_unroll(i, 0, PACKED_BATCH_SIZE)
		buf_lens[i] = pkts[i]->buf_len - pkts[i]->data_off;

	vhost_for_each_try_unroll(i, 0, PACKED_BATCH_SIZE) {
		if (unlikely(buf_lens[i] < (lens[i] - buf_offset)))
			goto free_buf;
	}

	vhost_for_each_try_unroll(i, 0, PACKED_BATCH_SIZE) {
		pkts[i]->pkt_len = descs[avail_idx + i].len - buf_offset;
		pkts[i]->data_len = pkts[i]->pkt_len;
		ids[i] = descs[avail_idx + i].id;
	}

	if (virtio_net_with_host_offload(dev)) {
		vhost_for_each_try_unroll(i, 0, PACKED_BATCH_SIZE) {
			hdr = (struct virtio_net_hdr *)(desc_addrs[i]);
			vhost_dequeue_offload(hdr, pkts[i]);
		}
	}

	return 0;

free_buf:
	for (i = 0; i < PACKED_BATCH_SIZE; i++)
		rte_pktmbuf_free(pkts[i]);

	return -1;
}