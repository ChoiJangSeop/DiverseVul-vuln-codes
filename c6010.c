static int get_v4l2_edid32(struct v4l2_edid *kp, struct v4l2_edid32 __user *up)
{
	u32 tmp;

	if (!access_ok(VERIFY_READ, up, sizeof(*up)) ||
	    get_user(kp->pad, &up->pad) ||
	    get_user(kp->start_block, &up->start_block) ||
	    get_user(kp->blocks, &up->blocks) ||
	    get_user(tmp, &up->edid) ||
	    copy_from_user(kp->reserved, up->reserved, sizeof(kp->reserved)))
		return -EFAULT;
	kp->edid = (__force u8 *)compat_ptr(tmp);
	return 0;
}