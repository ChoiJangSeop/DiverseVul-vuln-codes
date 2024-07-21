vhost_user_set_owner(struct virtio_net **pdev __rte_unused,
			struct VhostUserMsg *msg __rte_unused,
			int main_fd __rte_unused)
{
	return RTE_VHOST_MSG_RESULT_OK;
}