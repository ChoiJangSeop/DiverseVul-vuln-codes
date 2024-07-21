static int put_v4l2_window32(struct v4l2_window *kp, struct v4l2_window32 __user *up)
{
	struct v4l2_clip __user *kclips = kp->clips;
	struct v4l2_clip32 __user *uclips;
	u32 n = kp->clipcount;
	compat_caddr_t p;

	if (copy_to_user(&up->w, &kp->w, sizeof(kp->w)) ||
	    put_user(kp->field, &up->field) ||
	    put_user(kp->chromakey, &up->chromakey) ||
	    put_user(kp->clipcount, &up->clipcount) ||
	    put_user(kp->global_alpha, &up->global_alpha))
		return -EFAULT;

	if (!kp->clipcount)
		return 0;

	if (get_user(p, &up->clips))
		return -EFAULT;
	uclips = compat_ptr(p);
	while (n--) {
		if (copy_in_user(&uclips->c, &kclips->c, sizeof(uclips->c)))
			return -EFAULT;
		uclips++;
		kclips++;
	}
	return 0;
}