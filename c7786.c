vhost_user_set_vring_enable(struct virtio_net **pdev,
			struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	struct virtio_net *dev = *pdev;
	int enable = (int)msg->payload.state.num;
	int index = (int)msg->payload.state.index;
	struct rte_vdpa_device *vdpa_dev;
	int did = -1;

	RTE_LOG(INFO, VHOST_CONFIG,
		"set queue enable: %d to qp idx: %d\n",
		enable, index);

	did = dev->vdpa_dev_id;
	vdpa_dev = rte_vdpa_get_device(did);
	if (vdpa_dev && vdpa_dev->ops->set_vring_state)
		vdpa_dev->ops->set_vring_state(dev->vid, index, enable);

	if (dev->notify_ops->vring_state_changed)
		dev->notify_ops->vring_state_changed(dev->vid,
				index, enable);

	/* On disable, rings have to be stopped being processed. */
	if (!enable && dev->dequeue_zero_copy)
		drain_zmbuf_list(dev->virtqueue[index]);

	dev->virtqueue[index]->enabled = enable;

	return RTE_VHOST_MSG_RESULT_OK;
}