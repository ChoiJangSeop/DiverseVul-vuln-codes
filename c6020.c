static int __put_v4l2_format32(struct v4l2_format *kp, struct v4l2_format32 __user *up)
{
	if (put_user(kp->type, &up->type))
		return -EFAULT;

	switch (kp->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		return copy_to_user(&up->fmt.pix, &kp->fmt.pix,
				    sizeof(kp->fmt.pix)) ? -EFAULT : 0;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		return copy_to_user(&up->fmt.pix_mp, &kp->fmt.pix_mp,
				    sizeof(kp->fmt.pix_mp)) ? -EFAULT : 0;
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
		return put_v4l2_window32(&kp->fmt.win, &up->fmt.win);
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
		return copy_to_user(&up->fmt.vbi, &kp->fmt.vbi,
				    sizeof(kp->fmt.vbi)) ? -EFAULT : 0;
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		return copy_to_user(&up->fmt.sliced, &kp->fmt.sliced,
				    sizeof(kp->fmt.sliced)) ? -EFAULT : 0;
	case V4L2_BUF_TYPE_SDR_CAPTURE:
	case V4L2_BUF_TYPE_SDR_OUTPUT:
		return copy_to_user(&up->fmt.sdr, &kp->fmt.sdr,
				    sizeof(kp->fmt.sdr)) ? -EFAULT : 0;
	case V4L2_BUF_TYPE_META_CAPTURE:
		return copy_to_user(&up->fmt.meta, &kp->fmt.meta,
				    sizeof(kp->fmt.meta)) ? -EFAULT : 0;
	default:
		return -EINVAL;
	}
}