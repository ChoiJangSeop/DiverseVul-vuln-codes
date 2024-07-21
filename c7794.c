static int vhost_user_set_vring_err(struct virtio_net **pdev __rte_unused,
			struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	if (!(msg->payload.u64 & VHOST_USER_VRING_NOFD_MASK))
		close(msg->fds[0]);
	RTE_LOG(INFO, VHOST_CONFIG, "not implemented\n");

	return RTE_VHOST_MSG_RESULT_OK;
}