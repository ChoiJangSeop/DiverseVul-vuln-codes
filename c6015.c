static int put_v4l2_buffer32(struct v4l2_buffer *kp, struct v4l2_buffer32 __user *up)
{
	struct v4l2_plane32 __user *uplane32;
	struct v4l2_plane __user *uplane;
	compat_caddr_t p;
	int num_planes;
	int ret;

	if (!access_ok(VERIFY_WRITE, up, sizeof(*up)) ||
	    put_user(kp->index, &up->index) ||
	    put_user(kp->type, &up->type) ||
	    put_user(kp->flags, &up->flags) ||
	    put_user(kp->memory, &up->memory))
		return -EFAULT;

	if (put_user(kp->bytesused, &up->bytesused) ||
	    put_user(kp->field, &up->field) ||
	    put_user(kp->timestamp.tv_sec, &up->timestamp.tv_sec) ||
	    put_user(kp->timestamp.tv_usec, &up->timestamp.tv_usec) ||
	    copy_to_user(&up->timecode, &kp->timecode, sizeof(kp->timecode)) ||
	    put_user(kp->sequence, &up->sequence) ||
	    put_user(kp->reserved2, &up->reserved2) ||
	    put_user(kp->reserved, &up->reserved) ||
	    put_user(kp->length, &up->length))
		return -EFAULT;

	if (V4L2_TYPE_IS_MULTIPLANAR(kp->type)) {
		num_planes = kp->length;
		if (num_planes == 0)
			return 0;

		uplane = (__force struct v4l2_plane __user *)kp->m.planes;
		if (get_user(p, &up->m.planes))
			return -EFAULT;
		uplane32 = compat_ptr(p);

		while (--num_planes >= 0) {
			ret = put_v4l2_plane32(uplane, uplane32, kp->memory);
			if (ret)
				return ret;
			++uplane;
			++uplane32;
		}
	} else {
		switch (kp->memory) {
		case V4L2_MEMORY_MMAP:
		case V4L2_MEMORY_OVERLAY:
			if (put_user(kp->m.offset, &up->m.offset))
				return -EFAULT;
			break;
		case V4L2_MEMORY_USERPTR:
			if (put_user(kp->m.userptr, &up->m.userptr))
				return -EFAULT;
			break;
		case V4L2_MEMORY_DMABUF:
			if (put_user(kp->m.fd, &up->m.fd))
				return -EFAULT;
			break;
		}
	}

	return 0;
}