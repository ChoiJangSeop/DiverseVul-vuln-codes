vhost_user_set_protocol_features(struct virtio_net **pdev,
			struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	struct virtio_net *dev = *pdev;
	uint64_t protocol_features = msg->payload.u64;
	uint64_t slave_protocol_features = 0;

	rte_vhost_driver_get_protocol_features(dev->ifname,
			&slave_protocol_features);
	if (protocol_features & ~slave_protocol_features) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"(%d) received invalid protocol features.\n",
			dev->vid);
		return RTE_VHOST_MSG_RESULT_ERR;
	}

	dev->protocol_features = protocol_features;
	RTE_LOG(INFO, VHOST_CONFIG,
		"negotiated Vhost-user protocol features: 0x%" PRIx64 "\n",
		dev->protocol_features);

	return RTE_VHOST_MSG_RESULT_OK;
}