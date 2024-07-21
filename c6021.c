static int get_v4l2_create32(struct v4l2_create_buffers *kp, struct v4l2_create_buffers32 __user *up)
{
	if (!access_ok(VERIFY_READ, up, sizeof(*up)) ||
	    copy_from_user(kp, up, offsetof(struct v4l2_create_buffers32, format)))
		return -EFAULT;
	return __get_v4l2_format32(&kp->format, &up->format);
}