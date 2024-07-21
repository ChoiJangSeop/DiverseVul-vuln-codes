vhost_user_net_set_mtu(struct virtio_net **pdev, struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	struct virtio_net *dev = *pdev;
	if (msg->payload.u64 < VIRTIO_MIN_MTU ||
			msg->payload.u64 > VIRTIO_MAX_MTU) {
		RTE_LOG(ERR, VHOST_CONFIG, "Invalid MTU size (%"PRIu64")\n",
				msg->payload.u64);

		return RTE_VHOST_MSG_RESULT_ERR;
	}

	dev->mtu = msg->payload.u64;

	return RTE_VHOST_MSG_RESULT_OK;
}