vhost_user_set_mem_table(struct virtio_net **pdev, struct VhostUserMsg *msg,
			int main_fd)
{
	struct virtio_net *dev = *pdev;
	struct VhostUserMemory *memory = &msg->payload.memory;
	struct rte_vhost_mem_region *reg;
	void *mmap_addr;
	uint64_t mmap_size;
	uint64_t mmap_offset;
	uint64_t alignment;
	uint32_t i;
	int populate;
	int fd;

	if (memory->nregions > VHOST_MEMORY_MAX_NREGIONS) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"too many memory regions (%u)\n", memory->nregions);
		return RTE_VHOST_MSG_RESULT_ERR;
	}

	if (dev->mem && !vhost_memory_changed(memory, dev->mem)) {
		RTE_LOG(INFO, VHOST_CONFIG,
			"(%d) memory regions not changed\n", dev->vid);

		for (i = 0; i < memory->nregions; i++)
			close(msg->fds[i]);

		return RTE_VHOST_MSG_RESULT_OK;
	}

	if (dev->mem) {
		free_mem_region(dev);
		rte_free(dev->mem);
		dev->mem = NULL;
	}

	/* Flush IOTLB cache as previous HVAs are now invalid */
	if (dev->features & (1ULL << VIRTIO_F_IOMMU_PLATFORM))
		for (i = 0; i < dev->nr_vring; i++)
			vhost_user_iotlb_flush_all(dev->virtqueue[i]);

	dev->nr_guest_pages = 0;
	if (!dev->guest_pages) {
		dev->max_guest_pages = 8;
		dev->guest_pages = malloc(dev->max_guest_pages *
						sizeof(struct guest_page));
		if (dev->guest_pages == NULL) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"(%d) failed to allocate memory "
				"for dev->guest_pages\n",
				dev->vid);
			return RTE_VHOST_MSG_RESULT_ERR;
		}
	}

	dev->mem = rte_zmalloc("vhost-mem-table", sizeof(struct rte_vhost_memory) +
		sizeof(struct rte_vhost_mem_region) * memory->nregions, 0);
	if (dev->mem == NULL) {
		RTE_LOG(ERR, VHOST_CONFIG,
			"(%d) failed to allocate memory for dev->mem\n",
			dev->vid);
		return RTE_VHOST_MSG_RESULT_ERR;
	}
	dev->mem->nregions = memory->nregions;

	for (i = 0; i < memory->nregions; i++) {
		fd  = msg->fds[i];
		reg = &dev->mem->regions[i];

		reg->guest_phys_addr = memory->regions[i].guest_phys_addr;
		reg->guest_user_addr = memory->regions[i].userspace_addr;
		reg->size            = memory->regions[i].memory_size;
		reg->fd              = fd;

		mmap_offset = memory->regions[i].mmap_offset;

		/* Check for memory_size + mmap_offset overflow */
		if (mmap_offset >= -reg->size) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"mmap_offset (%#"PRIx64") and memory_size "
				"(%#"PRIx64") overflow\n",
				mmap_offset, reg->size);
			goto err_mmap;
		}

		mmap_size = reg->size + mmap_offset;

		/* mmap() without flag of MAP_ANONYMOUS, should be called
		 * with length argument aligned with hugepagesz at older
		 * longterm version Linux, like 2.6.32 and 3.2.72, or
		 * mmap() will fail with EINVAL.
		 *
		 * to avoid failure, make sure in caller to keep length
		 * aligned.
		 */
		alignment = get_blk_size(fd);
		if (alignment == (uint64_t)-1) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"couldn't get hugepage size through fstat\n");
			goto err_mmap;
		}
		mmap_size = RTE_ALIGN_CEIL(mmap_size, alignment);

		populate = (dev->dequeue_zero_copy) ? MAP_POPULATE : 0;
		mmap_addr = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
				 MAP_SHARED | populate, fd, 0);

		if (mmap_addr == MAP_FAILED) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"mmap region %u failed.\n", i);
			goto err_mmap;
		}

		reg->mmap_addr = mmap_addr;
		reg->mmap_size = mmap_size;
		reg->host_user_addr = (uint64_t)(uintptr_t)mmap_addr +
				      mmap_offset;

		if (dev->dequeue_zero_copy)
			if (add_guest_pages(dev, reg, alignment) < 0) {
				RTE_LOG(ERR, VHOST_CONFIG,
					"adding guest pages to region %u failed.\n",
					i);
				goto err_mmap;
			}

		RTE_LOG(INFO, VHOST_CONFIG,
			"guest memory region %u, size: 0x%" PRIx64 "\n"
			"\t guest physical addr: 0x%" PRIx64 "\n"
			"\t guest virtual  addr: 0x%" PRIx64 "\n"
			"\t host  virtual  addr: 0x%" PRIx64 "\n"
			"\t mmap addr : 0x%" PRIx64 "\n"
			"\t mmap size : 0x%" PRIx64 "\n"
			"\t mmap align: 0x%" PRIx64 "\n"
			"\t mmap off  : 0x%" PRIx64 "\n",
			i, reg->size,
			reg->guest_phys_addr,
			reg->guest_user_addr,
			reg->host_user_addr,
			(uint64_t)(uintptr_t)mmap_addr,
			mmap_size,
			alignment,
			mmap_offset);

		if (dev->postcopy_listening) {
			/*
			 * We haven't a better way right now than sharing
			 * DPDK's virtual address with Qemu, so that Qemu can
			 * retrieve the region offset when handling userfaults.
			 */
			memory->regions[i].userspace_addr =
				reg->host_user_addr;
		}
	}
	if (dev->postcopy_listening) {
		/* Send the addresses back to qemu */
		msg->fd_num = 0;
		send_vhost_reply(main_fd, msg);

		/* Wait for qemu to acknolwedge it's got the addresses
		 * we've got to wait before we're allowed to generate faults.
		 */
		VhostUserMsg ack_msg;
		if (read_vhost_message(main_fd, &ack_msg) <= 0) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"Failed to read qemu ack on postcopy set-mem-table\n");
			goto err_mmap;
		}
		if (ack_msg.request.master != VHOST_USER_SET_MEM_TABLE) {
			RTE_LOG(ERR, VHOST_CONFIG,
				"Bad qemu ack on postcopy set-mem-table (%d)\n",
				ack_msg.request.master);
			goto err_mmap;
		}

		/* Now userfault register and we can use the memory */
		for (i = 0; i < memory->nregions; i++) {
#ifdef RTE_LIBRTE_VHOST_POSTCOPY
			reg = &dev->mem->regions[i];
			struct uffdio_register reg_struct;

			/*
			 * Let's register all the mmap'ed area to ensure
			 * alignment on page boundary.
			 */
			reg_struct.range.start =
				(uint64_t)(uintptr_t)reg->mmap_addr;
			reg_struct.range.len = reg->mmap_size;
			reg_struct.mode = UFFDIO_REGISTER_MODE_MISSING;

			if (ioctl(dev->postcopy_ufd, UFFDIO_REGISTER,
						&reg_struct)) {
				RTE_LOG(ERR, VHOST_CONFIG,
					"Failed to register ufd for region %d: (ufd = %d) %s\n",
					i, dev->postcopy_ufd,
					strerror(errno));
				goto err_mmap;
			}
			RTE_LOG(INFO, VHOST_CONFIG,
				"\t userfaultfd registered for range : "
				"%" PRIx64 " - %" PRIx64 "\n",
				(uint64_t)reg_struct.range.start,
				(uint64_t)reg_struct.range.start +
				(uint64_t)reg_struct.range.len - 1);
#else
			goto err_mmap;
#endif
		}
	}

	for (i = 0; i < dev->nr_vring; i++) {
		struct vhost_virtqueue *vq = dev->virtqueue[i];

		if (vq->desc || vq->avail || vq->used) {
			/*
			 * If the memory table got updated, the ring addresses
			 * need to be translated again as virtual addresses have
			 * changed.
			 */
			vring_invalidate(dev, vq);

			dev = translate_ring_addresses(dev, i);
			if (!dev) {
				dev = *pdev;
				goto err_mmap;
			}

			*pdev = dev;
		}
	}

	dump_guest_pages(dev);

	return RTE_VHOST_MSG_RESULT_OK;

err_mmap:
	free_mem_region(dev);
	rte_free(dev->mem);
	dev->mem = NULL;
	return RTE_VHOST_MSG_RESULT_ERR;
}