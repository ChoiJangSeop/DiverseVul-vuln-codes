static int vhost_user_set_log_fd(struct virtio_net **pdev __rte_unused,
			struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	close(msg->fds[0]);
	RTE_LOG(INFO, VHOST_CONFIG, "not implemented.\n");

	return RTE_VHOST_MSG_RESULT_OK;
}