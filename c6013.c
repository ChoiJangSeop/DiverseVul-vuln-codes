static int put_v4l2_standard32(struct v4l2_standard *kp, struct v4l2_standard32 __user *up)
{
	if (!access_ok(VERIFY_WRITE, up, sizeof(*up)) ||
	    put_user(kp->index, &up->index) ||
	    put_user(kp->id, &up->id) ||
	    copy_to_user(up->name, kp->name, sizeof(up->name)) ||
	    copy_to_user(&up->frameperiod, &kp->frameperiod,
			 sizeof(kp->frameperiod)) ||
	    put_user(kp->framelines, &up->framelines) ||
	    copy_to_user(up->reserved, kp->reserved, sizeof(kp->reserved)))
		return -EFAULT;
	return 0;
}