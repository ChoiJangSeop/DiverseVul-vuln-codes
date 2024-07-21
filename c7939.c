static int get_indirect(struct vhost_virtqueue *vq,
			struct iovec iov[], unsigned int iov_size,
			unsigned int *out_num, unsigned int *in_num,
			struct vhost_log *log, unsigned int *log_num,
			struct vring_desc *indirect)
{
	struct vring_desc desc;
	unsigned int i = 0, count, found = 0;
	u32 len = vhost32_to_cpu(vq, indirect->len);
	struct iov_iter from;
	int ret, access;

	/* Sanity check */
	if (unlikely(len % sizeof desc)) {
		vq_err(vq, "Invalid length in indirect descriptor: "
		       "len 0x%llx not multiple of 0x%zx\n",
		       (unsigned long long)len,
		       sizeof desc);
		return -EINVAL;
	}

	ret = translate_desc(vq, vhost64_to_cpu(vq, indirect->addr), len, vq->indirect,
			     UIO_MAXIOV, VHOST_ACCESS_RO);
	if (unlikely(ret < 0)) {
		if (ret != -EAGAIN)
			vq_err(vq, "Translation failure %d in indirect.\n", ret);
		return ret;
	}
	iov_iter_init(&from, READ, vq->indirect, ret, len);

	/* We will use the result as an address to read from, so most
	 * architectures only need a compiler barrier here. */
	read_barrier_depends();

	count = len / sizeof desc;
	/* Buffers are chained via a 16 bit next field, so
	 * we can have at most 2^16 of these. */
	if (unlikely(count > USHRT_MAX + 1)) {
		vq_err(vq, "Indirect buffer length too big: %d\n",
		       indirect->len);
		return -E2BIG;
	}

	do {
		unsigned iov_count = *in_num + *out_num;
		if (unlikely(++found > count)) {
			vq_err(vq, "Loop detected: last one at %u "
			       "indirect size %u\n",
			       i, count);
			return -EINVAL;
		}
		if (unlikely(!copy_from_iter_full(&desc, sizeof(desc), &from))) {
			vq_err(vq, "Failed indirect descriptor: idx %d, %zx\n",
			       i, (size_t)vhost64_to_cpu(vq, indirect->addr) + i * sizeof desc);
			return -EINVAL;
		}
		if (unlikely(desc.flags & cpu_to_vhost16(vq, VRING_DESC_F_INDIRECT))) {
			vq_err(vq, "Nested indirect descriptor: idx %d, %zx\n",
			       i, (size_t)vhost64_to_cpu(vq, indirect->addr) + i * sizeof desc);
			return -EINVAL;
		}

		if (desc.flags & cpu_to_vhost16(vq, VRING_DESC_F_WRITE))
			access = VHOST_ACCESS_WO;
		else
			access = VHOST_ACCESS_RO;

		ret = translate_desc(vq, vhost64_to_cpu(vq, desc.addr),
				     vhost32_to_cpu(vq, desc.len), iov + iov_count,
				     iov_size - iov_count, access);
		if (unlikely(ret < 0)) {
			if (ret != -EAGAIN)
				vq_err(vq, "Translation failure %d indirect idx %d\n",
					ret, i);
			return ret;
		}
		/* If this is an input descriptor, increment that count. */
		if (access == VHOST_ACCESS_WO) {
			*in_num += ret;
			if (unlikely(log)) {
				log[*log_num].addr = vhost64_to_cpu(vq, desc.addr);
				log[*log_num].len = vhost32_to_cpu(vq, desc.len);
				++*log_num;
			}
		} else {
			/* If it's an output descriptor, they're all supposed
			 * to come before any input descriptors. */
			if (unlikely(*in_num)) {
				vq_err(vq, "Indirect descriptor "
				       "has out after in: idx %d\n", i);
				return -EINVAL;
			}
			*out_num += ret;
		}
	} while ((i = next_desc(vq, &desc)) != -1);
	return 0;
}