vhost_user_set_vring_num(struct virtio_net **pdev,
			struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	struct virtio_net *dev = *pdev;
	struct vhost_virtqueue *vq = dev->virtqueue[msg->payload.state.index];

	vq->size = msg->payload.state.num;

	/* VIRTIO 1.0, 2.4 Virtqueues says:
	 *
	 *   Queue Size value is always a power of 2. The maximum Queue Size
	 *   value is 32768.
	 *
	 * VIRTIO 1.1 2.7 Virtqueues says:
	 *
	 *   Packed virtqueues support up to 2^15 entries each.
	 */
	if (!vq_is_packed(dev)) {
		if (vq->size & (vq->size - 1)) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"invalid virtqueue size %u\n", vq->size);
			return RTE_VHOST_MSG_RESULT_ERR;
		}
	}

	if (vq->size > 32768) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"invalid virtqueue size %u\n", vq->size);
		return RTE_VHOST_MSG_RESULT_ERR;
	}

	if (dev->dequeue_zero_copy) {
		vq->nr_zmbuf = 0;
		vq->last_zmbuf_idx = 0;
		vq->zmbuf_size = vq->size;
		vq->zmbufs = rte_zmalloc(NULL, vq->zmbuf_size *
					 sizeof(struct zcopy_mbuf), 0);
		if (vq->zmbufs == NULL) {
			RTE_LOG(WARNING, VHOST_CONFIG,
				"failed to allocate mem for zero copy; "
				"zero copy is force disabled\n");
			dev->dequeue_zero_copy = 0;
		}
		TAILQ_INIT(&vq->zmbuf_list);
	}

	if (vq_is_packed(dev)) {
		vq->shadow_used_packed = rte_malloc(NULL,
				vq->size *
				sizeof(struct vring_used_elem_packed),
				RTE_CACHE_LINE_SIZE);
		if (!vq->shadow_used_packed) {
			RTE_LOG(ERR, VHOST_CONFIG,
					"failed to allocate memory for shadow used ring.\n");
			return RTE_VHOST_MSG_RESULT_ERR;
		}

	} else {
		vq->shadow_used_split = rte_malloc(NULL,
				vq->size * sizeof(struct vring_used_elem),
				RTE_CACHE_LINE_SIZE);
		if (!vq->shadow_used_split) {
			RTE_LOG(ERR, VHOST_CONFIG,
					"failed to allocate memory for shadow used ring.\n");
			return RTE_VHOST_MSG_RESULT_ERR;
		}
	}

	vq->batch_copy_elems = rte_malloc(NULL,
				vq->size * sizeof(struct batch_copy_elem),
				RTE_CACHE_LINE_SIZE);
	if (!vq->batch_copy_elems) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"failed to allocate memory for batching copy.\n");
		return RTE_VHOST_MSG_RESULT_ERR;
	}

	return RTE_VHOST_MSG_RESULT_OK;
}