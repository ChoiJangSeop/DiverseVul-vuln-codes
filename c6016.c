static int put_v4l2_edid32(struct v4l2_edid *kp, struct v4l2_edid32 __user *up)
{
	u32 tmp = (u32)((unsigned long)kp->edid);

	if (!access_ok(VERIFY_WRITE, up, sizeof(*up)) ||
	    put_user(kp->pad, &up->pad) ||
	    put_user(kp->start_block, &up->start_block) ||
	    put_user(kp->blocks, &up->blocks) ||
	    put_user(tmp, &up->edid) ||
	    copy_to_user(up->reserved, kp->reserved, sizeof(up->reserved)))
		return -EFAULT;
	return 0;
}