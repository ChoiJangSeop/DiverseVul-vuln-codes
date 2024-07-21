vhost_user_postcopy_end(struct virtio_net **pdev, struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	struct virtio_net *dev = *pdev;

	dev->postcopy_listening = 0;
	if (dev->postcopy_ufd >= 0) {
		close(dev->postcopy_ufd);
		dev->postcopy_ufd = -1;
	}

	msg->payload.u64 = 0;
	msg->size = sizeof(msg->payload.u64);
	msg->fd_num = 0;

	return RTE_VHOST_MSG_RESULT_REPLY;
}