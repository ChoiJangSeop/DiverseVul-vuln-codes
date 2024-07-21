static int get_v4l2_standard32(struct v4l2_standard *kp, struct v4l2_standard32 __user *up)
{
	/* other fields are not set by the user, nor used by the driver */
	if (!access_ok(VERIFY_READ, up, sizeof(*up)) ||
	    get_user(kp->index, &up->index))
		return -EFAULT;
	return 0;
}