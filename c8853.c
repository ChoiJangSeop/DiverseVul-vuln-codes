vhost_backend_cleanup(struct virtio_net *dev)
{
	if (dev->mem) {
		free_mem_region(dev);
		rte_free(dev->mem);
		dev->mem = NULL;
	}

	rte_free(dev->guest_pages);
	dev->guest_pages = NULL;

	if (dev->log_addr) {
		munmap((void *)(uintptr_t)dev->log_addr, dev->log_size);
		dev->log_addr = 0;
	}

	if (dev->inflight_info) {
		if (dev->inflight_info->addr) {
			munmap(dev->inflight_info->addr,
			       dev->inflight_info->size);
			dev->inflight_info->addr = NULL;
		}

		if (dev->inflight_info->fd > 0) {
			close(dev->inflight_info->fd);
			dev->inflight_info->fd = -1;
		}

		free(dev->inflight_info);
		dev->inflight_info = NULL;
	}

	if (dev->slave_req_fd >= 0) {
		close(dev->slave_req_fd);
		dev->slave_req_fd = -1;
	}

	if (dev->postcopy_ufd >= 0) {
		close(dev->postcopy_ufd);
		dev->postcopy_ufd = -1;
	}

	dev->postcopy_listening = 0;
}