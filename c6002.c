static int get_v4l2_framebuffer32(struct v4l2_framebuffer *kp, struct v4l2_framebuffer32 __user *up)
{
	u32 tmp;

	if (!access_ok(VERIFY_READ, up, sizeof(*up)) ||
	    get_user(tmp, &up->base) ||
	    get_user(kp->capability, &up->capability) ||
	    get_user(kp->flags, &up->flags) ||
	    copy_from_user(&kp->fmt, &up->fmt, sizeof(up->fmt)))
		return -EFAULT;
	kp->base = (__force void *)compat_ptr(tmp);
	return 0;
}