vhost_user_get_queue_num(struct virtio_net **pdev, struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	struct virtio_net *dev = *pdev;
	uint32_t queue_num = 0;

	rte_vhost_driver_get_queue_num(dev->ifname, &queue_num);

	msg->payload.u64 = (uint64_t)queue_num;
	msg->size = sizeof(msg->payload.u64);
	msg->fd_num = 0;

	return RTE_VHOST_MSG_RESULT_REPLY;
}