static int put_v4l2_create32(struct v4l2_create_buffers *kp, struct v4l2_create_buffers32 __user *up)
{
	if (!access_ok(VERIFY_WRITE, up, sizeof(*up)) ||
	    copy_to_user(up, kp, offsetof(struct v4l2_create_buffers32, format)) ||
	    copy_to_user(up->reserved, kp->reserved, sizeof(kp->reserved)))
		return -EFAULT;
	return __put_v4l2_format32(&kp->format, &up->format);
}