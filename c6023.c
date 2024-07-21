static int __get_v4l2_format32(struct v4l2_format *kp, struct v4l2_format32 __user *up)
{
	if (get_user(kp->type, &up->type))
		return -EFAULT;

	switch (kp->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		return copy_from_user(&kp->fmt.pix, &up->fmt.pix,
				      sizeof(kp->fmt.pix)) ? -EFAULT : 0;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		return copy_from_user(&kp->fmt.pix_mp, &up->fmt.pix_mp,
				      sizeof(kp->fmt.pix_mp)) ? -EFAULT : 0;
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
		return get_v4l2_window32(&kp->fmt.win, &up->fmt.win);
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
		return copy_from_user(&kp->fmt.vbi, &up->fmt.vbi,
				      sizeof(kp->fmt.vbi)) ? -EFAULT : 0;
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		return copy_from_user(&kp->fmt.sliced, &up->fmt.sliced,
				      sizeof(kp->fmt.sliced)) ? -EFAULT : 0;
	case V4L2_BUF_TYPE_SDR_CAPTURE:
	case V4L2_BUF_TYPE_SDR_OUTPUT:
		return copy_from_user(&kp->fmt.sdr, &up->fmt.sdr,
				      sizeof(kp->fmt.sdr)) ? -EFAULT : 0;
	case V4L2_BUF_TYPE_META_CAPTURE:
		return copy_from_user(&kp->fmt.meta, &up->fmt.meta,
				      sizeof(kp->fmt.meta)) ? -EFAULT : 0;
	default:
		return -EINVAL;
	}
}