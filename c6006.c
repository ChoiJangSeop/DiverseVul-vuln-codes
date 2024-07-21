static long do_video_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	union {
		struct v4l2_format v2f;
		struct v4l2_buffer v2b;
		struct v4l2_framebuffer v2fb;
		struct v4l2_input v2i;
		struct v4l2_standard v2s;
		struct v4l2_ext_controls v2ecs;
		struct v4l2_event v2ev;
		struct v4l2_create_buffers v2crt;
		struct v4l2_edid v2edid;
		unsigned long vx;
		int vi;
	} karg;
	void __user *up = compat_ptr(arg);
	int compatible_arg = 1;
	long err = 0;

	/* First, convert the command. */
	switch (cmd) {
	case VIDIOC_G_FMT32: cmd = VIDIOC_G_FMT; break;
	case VIDIOC_S_FMT32: cmd = VIDIOC_S_FMT; break;
	case VIDIOC_QUERYBUF32: cmd = VIDIOC_QUERYBUF; break;
	case VIDIOC_G_FBUF32: cmd = VIDIOC_G_FBUF; break;
	case VIDIOC_S_FBUF32: cmd = VIDIOC_S_FBUF; break;
	case VIDIOC_QBUF32: cmd = VIDIOC_QBUF; break;
	case VIDIOC_DQBUF32: cmd = VIDIOC_DQBUF; break;
	case VIDIOC_ENUMSTD32: cmd = VIDIOC_ENUMSTD; break;
	case VIDIOC_ENUMINPUT32: cmd = VIDIOC_ENUMINPUT; break;
	case VIDIOC_TRY_FMT32: cmd = VIDIOC_TRY_FMT; break;
	case VIDIOC_G_EXT_CTRLS32: cmd = VIDIOC_G_EXT_CTRLS; break;
	case VIDIOC_S_EXT_CTRLS32: cmd = VIDIOC_S_EXT_CTRLS; break;
	case VIDIOC_TRY_EXT_CTRLS32: cmd = VIDIOC_TRY_EXT_CTRLS; break;
	case VIDIOC_DQEVENT32: cmd = VIDIOC_DQEVENT; break;
	case VIDIOC_OVERLAY32: cmd = VIDIOC_OVERLAY; break;
	case VIDIOC_STREAMON32: cmd = VIDIOC_STREAMON; break;
	case VIDIOC_STREAMOFF32: cmd = VIDIOC_STREAMOFF; break;
	case VIDIOC_G_INPUT32: cmd = VIDIOC_G_INPUT; break;
	case VIDIOC_S_INPUT32: cmd = VIDIOC_S_INPUT; break;
	case VIDIOC_G_OUTPUT32: cmd = VIDIOC_G_OUTPUT; break;
	case VIDIOC_S_OUTPUT32: cmd = VIDIOC_S_OUTPUT; break;
	case VIDIOC_CREATE_BUFS32: cmd = VIDIOC_CREATE_BUFS; break;
	case VIDIOC_PREPARE_BUF32: cmd = VIDIOC_PREPARE_BUF; break;
	case VIDIOC_G_EDID32: cmd = VIDIOC_G_EDID; break;
	case VIDIOC_S_EDID32: cmd = VIDIOC_S_EDID; break;
	}

	switch (cmd) {
	case VIDIOC_OVERLAY:
	case VIDIOC_STREAMON:
	case VIDIOC_STREAMOFF:
	case VIDIOC_S_INPUT:
	case VIDIOC_S_OUTPUT:
		err = get_user(karg.vi, (s32 __user *)up);
		compatible_arg = 0;
		break;

	case VIDIOC_G_INPUT:
	case VIDIOC_G_OUTPUT:
		compatible_arg = 0;
		break;

	case VIDIOC_G_EDID:
	case VIDIOC_S_EDID:
		err = get_v4l2_edid32(&karg.v2edid, up);
		compatible_arg = 0;
		break;

	case VIDIOC_G_FMT:
	case VIDIOC_S_FMT:
	case VIDIOC_TRY_FMT:
		err = get_v4l2_format32(&karg.v2f, up);
		compatible_arg = 0;
		break;

	case VIDIOC_CREATE_BUFS:
		err = get_v4l2_create32(&karg.v2crt, up);
		compatible_arg = 0;
		break;

	case VIDIOC_PREPARE_BUF:
	case VIDIOC_QUERYBUF:
	case VIDIOC_QBUF:
	case VIDIOC_DQBUF:
		err = get_v4l2_buffer32(&karg.v2b, up);
		compatible_arg = 0;
		break;

	case VIDIOC_S_FBUF:
		err = get_v4l2_framebuffer32(&karg.v2fb, up);
		compatible_arg = 0;
		break;

	case VIDIOC_G_FBUF:
		compatible_arg = 0;
		break;

	case VIDIOC_ENUMSTD:
		err = get_v4l2_standard32(&karg.v2s, up);
		compatible_arg = 0;
		break;

	case VIDIOC_ENUMINPUT:
		err = get_v4l2_input32(&karg.v2i, up);
		compatible_arg = 0;
		break;

	case VIDIOC_G_EXT_CTRLS:
	case VIDIOC_S_EXT_CTRLS:
	case VIDIOC_TRY_EXT_CTRLS:
		err = get_v4l2_ext_controls32(file, &karg.v2ecs, up);
		compatible_arg = 0;
		break;
	case VIDIOC_DQEVENT:
		compatible_arg = 0;
		break;
	}
	if (err)
		return err;

	if (compatible_arg)
		err = native_ioctl(file, cmd, (unsigned long)up);
	else {
		mm_segment_t old_fs = get_fs();

		set_fs(KERNEL_DS);
		err = native_ioctl(file, cmd, (unsigned long)&karg);
		set_fs(old_fs);
	}

	if (err == -ENOTTY)
		return err;

	/* Special case: even after an error we need to put the
	   results back for these ioctls since the error_idx will
	   contain information on which control failed. */
	switch (cmd) {
	case VIDIOC_G_EXT_CTRLS:
	case VIDIOC_S_EXT_CTRLS:
	case VIDIOC_TRY_EXT_CTRLS:
		if (put_v4l2_ext_controls32(file, &karg.v2ecs, up))
			err = -EFAULT;
		break;
	case VIDIOC_S_EDID:
		if (put_v4l2_edid32(&karg.v2edid, up))
			err = -EFAULT;
		break;
	}
	if (err)
		return err;

	switch (cmd) {
	case VIDIOC_S_INPUT:
	case VIDIOC_S_OUTPUT:
	case VIDIOC_G_INPUT:
	case VIDIOC_G_OUTPUT:
		err = put_user(((s32)karg.vi), (s32 __user *)up);
		break;

	case VIDIOC_G_FBUF:
		err = put_v4l2_framebuffer32(&karg.v2fb, up);
		break;

	case VIDIOC_DQEVENT:
		err = put_v4l2_event32(&karg.v2ev, up);
		break;

	case VIDIOC_G_EDID:
		err = put_v4l2_edid32(&karg.v2edid, up);
		break;

	case VIDIOC_G_FMT:
	case VIDIOC_S_FMT:
	case VIDIOC_TRY_FMT:
		err = put_v4l2_format32(&karg.v2f, up);
		break;

	case VIDIOC_CREATE_BUFS:
		err = put_v4l2_create32(&karg.v2crt, up);
		break;

	case VIDIOC_PREPARE_BUF:
	case VIDIOC_QUERYBUF:
	case VIDIOC_QBUF:
	case VIDIOC_DQBUF:
		err = put_v4l2_buffer32(&karg.v2b, up);
		break;

	case VIDIOC_ENUMSTD:
		err = put_v4l2_standard32(&karg.v2s, up);
		break;

	case VIDIOC_ENUMINPUT:
		err = put_v4l2_input32(&karg.v2i, up);
		break;
	}
	return err;
}