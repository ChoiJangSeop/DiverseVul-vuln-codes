static int get_v4l2_buffer32(struct v4l2_buffer *kp, struct v4l2_buffer32 __user *up)
{
	struct v4l2_plane32 __user *uplane32;
	struct v4l2_plane __user *uplane;
	compat_caddr_t p;
	int ret;

	if (!access_ok(VERIFY_READ, up, sizeof(*up)) ||
	    get_user(kp->index, &up->index) ||
	    get_user(kp->type, &up->type) ||
	    get_user(kp->flags, &up->flags) ||
	    get_user(kp->memory, &up->memory) ||
	    get_user(kp->length, &up->length))
		return -EFAULT;

	if (V4L2_TYPE_IS_OUTPUT(kp->type))
		if (get_user(kp->bytesused, &up->bytesused) ||
		    get_user(kp->field, &up->field) ||
		    get_user(kp->timestamp.tv_sec, &up->timestamp.tv_sec) ||
		    get_user(kp->timestamp.tv_usec, &up->timestamp.tv_usec))
			return -EFAULT;

	if (V4L2_TYPE_IS_MULTIPLANAR(kp->type)) {
		unsigned int num_planes;

		if (kp->length == 0) {
			kp->m.planes = NULL;
			/* num_planes == 0 is legal, e.g. when userspace doesn't
			 * need planes array on DQBUF*/
			return 0;
		} else if (kp->length > VIDEO_MAX_PLANES) {
			return -EINVAL;
		}

		if (get_user(p, &up->m.planes))
			return -EFAULT;

		uplane32 = compat_ptr(p);
		if (!access_ok(VERIFY_READ, uplane32,
			       kp->length * sizeof(*uplane32)))
			return -EFAULT;

		/* We don't really care if userspace decides to kill itself
		 * by passing a very big num_planes value */
		uplane = compat_alloc_user_space(kp->length * sizeof(*uplane));
		kp->m.planes = (__force struct v4l2_plane *)uplane;

		for (num_planes = 0; num_planes < kp->length; num_planes++) {
			ret = get_v4l2_plane32(uplane, uplane32, kp->memory);
			if (ret)
				return ret;
			++uplane;
			++uplane32;
		}
	} else {
		switch (kp->memory) {
		case V4L2_MEMORY_MMAP:
		case V4L2_MEMORY_OVERLAY:
			if (get_user(kp->m.offset, &up->m.offset))
				return -EFAULT;
			break;
		case V4L2_MEMORY_USERPTR:
			{
				compat_long_t tmp;

				if (get_user(tmp, &up->m.userptr))
					return -EFAULT;

				kp->m.userptr = (unsigned long)compat_ptr(tmp);
			}
			break;
		case V4L2_MEMORY_DMABUF:
			if (get_user(kp->m.fd, &up->m.fd))
				return -EFAULT;
			break;
		}
	}

	return 0;
}