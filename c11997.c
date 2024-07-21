static int put_v4l2_window32(struct v4l2_window *kp, struct v4l2_window32 __user *up)
{
	if (copy_to_user(&up->w, &kp->w, sizeof(kp->w)) ||
	    put_user(kp->field, &up->field) ||
	    put_user(kp->chromakey, &up->chromakey) ||
	    put_user(kp->clipcount, &up->clipcount) ||
	    put_user(kp->global_alpha, &up->global_alpha))
		return -EFAULT;
	return 0;
}