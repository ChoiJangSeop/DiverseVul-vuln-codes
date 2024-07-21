static int get_v4l2_plane32(struct v4l2_plane __user *up, struct v4l2_plane32 __user *up32,
			    enum v4l2_memory memory)
{
	void __user *up_pln;
	compat_long_t p;

	if (copy_in_user(up, up32, 2 * sizeof(__u32)) ||
	    copy_in_user(&up->data_offset, &up32->data_offset,
			 sizeof(up->data_offset)))
		return -EFAULT;

	switch (memory) {
	case V4L2_MEMORY_MMAP:
	case V4L2_MEMORY_OVERLAY:
		if (copy_in_user(&up->m.mem_offset, &up32->m.mem_offset,
				 sizeof(up32->m.mem_offset)))
			return -EFAULT;
		break;
	case V4L2_MEMORY_USERPTR:
		if (get_user(p, &up32->m.userptr))
			return -EFAULT;
		up_pln = compat_ptr(p);
		if (put_user((unsigned long)up_pln, &up->m.userptr))
			return -EFAULT;
		break;
	case V4L2_MEMORY_DMABUF:
		if (copy_in_user(&up->m.fd, &up32->m.fd, sizeof(up32->m.fd)))
			return -EFAULT;
		break;
	}

	return 0;
}