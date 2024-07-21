vhost_user_get_vring_base(struct virtio_net **pdev,
			struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	struct virtio_net *dev = *pdev;
	struct vhost_virtqueue *vq = dev->virtqueue[msg->payload.state.index];
	uint64_t val;

	/* We have to stop the queue (virtio) if it is running. */
	vhost_destroy_device_notify(dev);

	dev->flags &= ~VIRTIO_DEV_READY;
	dev->flags &= ~VIRTIO_DEV_VDPA_CONFIGURED;

	/* Here we are safe to get the indexes */
	if (vq_is_packed(dev)) {
		/*
		 * Bit[0:14]: avail index
		 * Bit[15]: avail wrap counter
		 */
		val = vq->last_avail_idx & 0x7fff;
		val |= vq->avail_wrap_counter << 15;
		msg->payload.state.num = val;
	} else {
		msg->payload.state.num = vq->last_avail_idx;
	}

	RTE_LOG(INFO, VHOST_CONFIG,
		"vring base idx:%d file:%d\n", msg->payload.state.index,
		msg->payload.state.num);
	/*
	 * Based on current qemu vhost-user implementation, this message is
	 * sent and only sent in vhost_vring_stop.
	 * TODO: cleanup the vring, it isn't usable since here.
	 */
	if (vq->kickfd >= 0)
		close(vq->kickfd);

	vq->kickfd = VIRTIO_UNINITIALIZED_EVENTFD;

	if (vq->callfd >= 0)
		close(vq->callfd);

	vq->callfd = VIRTIO_UNINITIALIZED_EVENTFD;

	vq->signalled_used_valid = false;

	if (dev->dequeue_zero_copy)
		free_zmbufs(vq);
	if (vq_is_packed(dev)) {
		rte_free(vq->shadow_used_packed);
		vq->shadow_used_packed = NULL;
	} else {
		rte_free(vq->shadow_used_split);
		vq->shadow_used_split = NULL;
	}

	rte_free(vq->batch_copy_elems);
	vq->batch_copy_elems = NULL;

	msg->size = sizeof(msg->payload.state);
	msg->fd_num = 0;

	vring_invalidate(dev, vq);

	return RTE_VHOST_MSG_RESULT_REPLY;
}