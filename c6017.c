static int put_v4l2_framebuffer32(struct v4l2_framebuffer *kp, struct v4l2_framebuffer32 __user *up)
{
	u32 tmp = (u32)((unsigned long)kp->base);

	if (!access_ok(VERIFY_WRITE, up, sizeof(*up)) ||
	    put_user(tmp, &up->base) ||
	    put_user(kp->capability, &up->capability) ||
	    put_user(kp->flags, &up->flags) ||
	    copy_to_user(&up->fmt, &kp->fmt, sizeof(up->fmt)))
		return -EFAULT;
	return 0;
}