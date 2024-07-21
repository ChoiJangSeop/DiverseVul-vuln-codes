static int put_v4l2_event32(struct v4l2_event *kp, struct v4l2_event32 __user *up)
{
	if (!access_ok(VERIFY_WRITE, up, sizeof(*up)) ||
	    put_user(kp->type, &up->type) ||
	    copy_to_user(&up->u, &kp->u, sizeof(kp->u)) ||
	    put_user(kp->pending, &up->pending) ||
	    put_user(kp->sequence, &up->sequence) ||
	    put_user(kp->timestamp.tv_sec, &up->timestamp.tv_sec) ||
	    put_user(kp->timestamp.tv_nsec, &up->timestamp.tv_nsec) ||
	    put_user(kp->id, &up->id) ||
	    copy_to_user(up->reserved, kp->reserved, sizeof(kp->reserved)))
		return -EFAULT;
	return 0;
}