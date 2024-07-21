static int put_v4l2_format32(struct v4l2_format *kp, struct v4l2_format32 __user *up)
{
	if (!access_ok(VERIFY_WRITE, up, sizeof(*up)))
		return -EFAULT;
	return __put_v4l2_format32(kp, up);
}