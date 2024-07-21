vhost_user_get_features(struct virtio_net **pdev, struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	struct virtio_net *dev = *pdev;
	uint64_t features = 0;

	rte_vhost_driver_get_features(dev->ifname, &features);

	msg->payload.u64 = features;
	msg->size = sizeof(msg->payload.u64);
	msg->fd_num = 0;

	return RTE_VHOST_MSG_RESULT_REPLY;
}