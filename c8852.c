vhost_user_set_inflight_fd(struct virtio_net **pdev, VhostUserMsg *msg,
			   int main_fd __rte_unused)
{
	uint64_t mmap_size, mmap_offset;
	uint16_t num_queues, queue_size;
	struct virtio_net *dev = *pdev;
	uint32_t pervq_inflight_size;
	struct vhost_virtqueue *vq;
	void *addr;
	int fd, i;

	fd = msg->fds[0];
	if (msg->size != sizeof(msg->payload.inflight) || fd < 0) {
		VHOST_LOG_CONFIG(ERR,
			"invalid set_inflight_fd message size is %d,fd is %d\n",
			msg->size, fd);
		return RTE_VHOST_MSG_RESULT_ERR;
	}

	mmap_size = msg->payload.inflight.mmap_size;
	mmap_offset = msg->payload.inflight.mmap_offset;
	num_queues = msg->payload.inflight.num_queues;
	queue_size = msg->payload.inflight.queue_size;

	if (vq_is_packed(dev))
		pervq_inflight_size = get_pervq_shm_size_packed(queue_size);
	else
		pervq_inflight_size = get_pervq_shm_size_split(queue_size);

	VHOST_LOG_CONFIG(INFO,
		"set_inflight_fd mmap_size: %"PRIu64"\n", mmap_size);
	VHOST_LOG_CONFIG(INFO,
		"set_inflight_fd mmap_offset: %"PRIu64"\n", mmap_offset);
	VHOST_LOG_CONFIG(INFO,
		"set_inflight_fd num_queues: %u\n", num_queues);
	VHOST_LOG_CONFIG(INFO,
		"set_inflight_fd queue_size: %u\n", queue_size);
	VHOST_LOG_CONFIG(INFO,
		"set_inflight_fd fd: %d\n", fd);
	VHOST_LOG_CONFIG(INFO,
		"set_inflight_fd pervq_inflight_size: %d\n",
		pervq_inflight_size);

	if (!dev->inflight_info) {
		dev->inflight_info = calloc(1,
					    sizeof(struct inflight_mem_info));
		if (dev->inflight_info == NULL) {
			VHOST_LOG_CONFIG(ERR,
				"failed to alloc dev inflight area\n");
			return RTE_VHOST_MSG_RESULT_ERR;
		}
	}

	if (dev->inflight_info->addr)
		munmap(dev->inflight_info->addr, dev->inflight_info->size);

	addr = mmap(0, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		    fd, mmap_offset);
	if (addr == MAP_FAILED) {
		VHOST_LOG_CONFIG(ERR, "failed to mmap share memory.\n");
		return RTE_VHOST_MSG_RESULT_ERR;
	}

	if (dev->inflight_info->fd)
		close(dev->inflight_info->fd);

	dev->inflight_info->fd = fd;
	dev->inflight_info->addr = addr;
	dev->inflight_info->size = mmap_size;

	for (i = 0; i < num_queues; i++) {
		vq = dev->virtqueue[i];
		if (vq_is_packed(dev)) {
			vq->inflight_packed = addr;
			vq->inflight_packed->desc_num = queue_size;
		} else {
			vq->inflight_split = addr;
			vq->inflight_split->desc_num = queue_size;
		}
		addr = (void *)((char *)addr + pervq_inflight_size);
	}

	return RTE_VHOST_MSG_RESULT_OK;
}