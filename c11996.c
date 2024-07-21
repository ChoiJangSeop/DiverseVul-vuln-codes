static int get_v4l2_window32(struct v4l2_window *kp, struct v4l2_window32 __user *up)
{
	if (!access_ok(VERIFY_READ, up, sizeof(*up)) ||
	    copy_from_user(&kp->w, &up->w, sizeof(up->w)) ||
	    get_user(kp->field, &up->field) ||
	    get_user(kp->chromakey, &up->chromakey) ||
	    get_user(kp->clipcount, &up->clipcount) ||
	    get_user(kp->global_alpha, &up->global_alpha))
		return -EFAULT;
	if (kp->clipcount > 2048)
		return -EINVAL;
	if (kp->clipcount) {
		struct v4l2_clip32 __user *uclips;
		struct v4l2_clip __user *kclips;
		int n = kp->clipcount;
		compat_caddr_t p;

		if (get_user(p, &up->clips))
			return -EFAULT;
		uclips = compat_ptr(p);
		kclips = compat_alloc_user_space(n * sizeof(*kclips));
		kp->clips = kclips;
		while (--n >= 0) {
			if (copy_in_user(&kclips->c, &uclips->c, sizeof(uclips->c)))
				return -EFAULT;
			if (put_user(n ? kclips + 1 : NULL, &kclips->next))
				return -EFAULT;
			uclips += 1;
			kclips += 1;
		}
	} else
		kp->clips = NULL;
	return 0;
}