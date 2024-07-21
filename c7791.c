vhost_user_set_postcopy_advise(struct virtio_net **pdev,
			struct VhostUserMsg *msg,
			int main_fd __rte_unused)
{
	struct virtio_net *dev = *pdev;
#ifdef RTE_LIBRTE_VHOST_POSTCOPY
	struct uffdio_api api_struct;

	dev->postcopy_ufd = syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);

	if (dev->postcopy_ufd == -1) {
		RTE_LOG(ERR, VHOST_CONFIG, "Userfaultfd not available: %s\n",
			strerror(errno));
		return RTE_VHOST_MSG_RESULT_ERR;
	}
	api_struct.api = UFFD_API;
	api_struct.features = 0;
	if (ioctl(dev->postcopy_ufd, UFFDIO_API, &api_struct)) {
		RTE_LOG(ERR, VHOST_CONFIG, "UFFDIO_API ioctl failure: %s\n",
			strerror(errno));
		close(dev->postcopy_ufd);
		dev->postcopy_ufd = -1;
		return RTE_VHOST_MSG_RESULT_ERR;
	}
	msg->fds[0] = dev->postcopy_ufd;
	msg->fd_num = 1;

	return RTE_VHOST_MSG_RESULT_REPLY;
#else
	dev->postcopy_ufd = -1;
	msg->fd_num = 0;

	return RTE_VHOST_MSG_RESULT_ERR;
#endif
}