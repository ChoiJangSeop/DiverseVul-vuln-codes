vhost_user_reset_owner(struct virtio_net **pdev,
			struct VhostUserMsg *msg __rte_unused,
			int main_fd __rte_unused)
{
	struct virtio_net *dev = *pdev;
	vhost_destroy_device_notify(dev);

	cleanup_device(dev, 0);
	reset_device(dev);
	return RTE_VHOST_MSG_RESULT_OK;
}