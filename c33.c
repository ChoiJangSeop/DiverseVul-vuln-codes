static int pwc_video_close(struct inode *inode, struct file *file)
{
	struct video_device *vdev = file->private_data;
	struct pwc_device *pdev;
	int i;

	PWC_DEBUG_OPEN(">> video_close called(vdev = 0x%p).\n", vdev);

	pdev = (struct pwc_device *)vdev->priv;
	if (pdev->vopen == 0)
		PWC_DEBUG_MODULE("video_close() called on closed device?\n");

	/* Dump statistics, but only if a reasonable amount of frames were
	   processed (to prevent endless log-entries in case of snap-shot
	   programs)
	 */
	if (pdev->vframe_count > 20)
		PWC_DEBUG_MODULE("Closing video device: %d frames received, dumped %d frames, %d frames with errors.\n", pdev->vframe_count, pdev->vframes_dumped, pdev->vframes_error);

	if (DEVICE_USE_CODEC1(pdev->type))
	    pwc_dec1_exit();
	else
	    pwc_dec23_exit();

	pwc_isoc_cleanup(pdev);
	pwc_free_buffers(pdev);

	/* Turn off LEDS and power down camera, but only when not unplugged */
	if (pdev->error_status != EPIPE) {
		/* Turn LEDs off */
		if (pwc_set_leds(pdev, 0, 0) < 0)
			PWC_DEBUG_MODULE("Failed to set LED on/off time.\n");
		if (power_save) {
			i = pwc_camera_power(pdev, 0);
			if (i < 0)
				PWC_ERROR("Failed to power down camera (%d)\n", i);
		}
	}
	pdev->vopen--;
	PWC_DEBUG_OPEN("<< video_close() vopen=%d\n", pdev->vopen);
	return 0;
}