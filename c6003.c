static int get_v4l2_ext_controls32(struct file *file,
				   struct v4l2_ext_controls *kp,
				   struct v4l2_ext_controls32 __user *up)
{
	struct v4l2_ext_control32 __user *ucontrols;
	struct v4l2_ext_control __user *kcontrols;
	unsigned int n;
	compat_caddr_t p;

	if (!access_ok(VERIFY_READ, up, sizeof(*up)) ||
	    get_user(kp->which, &up->which) ||
	    get_user(kp->count, &up->count) ||
	    get_user(kp->error_idx, &up->error_idx) ||
	    copy_from_user(kp->reserved, up->reserved, sizeof(kp->reserved)))
		return -EFAULT;
	if (kp->count == 0) {
		kp->controls = NULL;
		return 0;
	} else if (kp->count > V4L2_CID_MAX_CTRLS) {
		return -EINVAL;
	}
	if (get_user(p, &up->controls))
		return -EFAULT;
	ucontrols = compat_ptr(p);
	if (!access_ok(VERIFY_READ, ucontrols, kp->count * sizeof(*ucontrols)))
		return -EFAULT;
	kcontrols = compat_alloc_user_space(kp->count * sizeof(*kcontrols));
	kp->controls = (__force struct v4l2_ext_control *)kcontrols;
	for (n = 0; n < kp->count; n++) {
		u32 id;

		if (copy_in_user(kcontrols, ucontrols, sizeof(*ucontrols)))
			return -EFAULT;
		if (get_user(id, &kcontrols->id))
			return -EFAULT;
		if (ctrl_is_pointer(file, id)) {
			void __user *s;

			if (get_user(p, &ucontrols->string))
				return -EFAULT;
			s = compat_ptr(p);
			if (put_user(s, &kcontrols->string))
				return -EFAULT;
		}
		ucontrols++;
		kcontrols++;
	}
	return 0;
}