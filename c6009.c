static int put_v4l2_plane32(struct v4l2_plane __user *up, struct v4l2_plane32 __user *up32,
			    enum v4l2_memory memory)
{
	unsigned long p;

	if (copy_in_user(up32, up, 2 * sizeof(__u32)) ||
	    copy_in_user(&up32->data_offset, &up->data_offset,
			 sizeof(up->data_offset)))
		return -EFAULT;

	switch (memory) {
	case V4L2_MEMORY_MMAP:
	case V4L2_MEMORY_OVERLAY:
		if (copy_in_user(&up32->m.mem_offset, &up->m.mem_offset,
				 sizeof(up->m.mem_offset)))
			return -EFAULT;
		break;
	case V4L2_MEMORY_USERPTR:
		if (get_user(p, &up->m.userptr) ||
		    put_user((compat_ulong_t)ptr_to_compat((__force void *)p),
			     &up32->m.userptr))
			return -EFAULT;
		break;
	case V4L2_MEMORY_DMABUF:
		if (copy_in_user(&up32->m.fd, &up->m.fd,
				 sizeof(up->m.fd)))
			return -EFAULT;
		break;
	}

	return 0;
}