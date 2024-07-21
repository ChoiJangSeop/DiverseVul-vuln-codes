static int put_v4l2_ext_controls32(struct file *file,
				   struct v4l2_ext_controls *kp,
				   struct v4l2_ext_controls32 __user *up)
{
	struct v4l2_ext_control32 __user *ucontrols;
	struct v4l2_ext_control __user *kcontrols =
		(__force struct v4l2_ext_control __user *)kp->controls;
	int n = kp->count;
	compat_caddr_t p;

	if (!access_ok(VERIFY_WRITE, up, sizeof(*up)) ||
	    put_user(kp->which, &up->which) ||
	    put_user(kp->count, &up->count) ||
	    put_user(kp->error_idx, &up->error_idx) ||
	    copy_to_user(up->reserved, kp->reserved, sizeof(up->reserved)))
		return -EFAULT;
	if (!kp->count)
		return 0;

	if (get_user(p, &up->controls))
		return -EFAULT;
	ucontrols = compat_ptr(p);
	if (!access_ok(VERIFY_WRITE, ucontrols, n * sizeof(*ucontrols)))
		return -EFAULT;

	while (--n >= 0) {
		unsigned size = sizeof(*ucontrols);
		u32 id;

		if (get_user(id, &kcontrols->id))
			return -EFAULT;
		/* Do not modify the pointer when copying a pointer control.
		   The contents of the pointer was changed, not the pointer
		   itself. */
		if (ctrl_is_pointer(file, id))
			size -= sizeof(ucontrols->value64);
		if (copy_in_user(ucontrols, kcontrols, size))
			return -EFAULT;
		ucontrols++;
		kcontrols++;
	}
	return 0;
}