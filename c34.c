static void usb_pwc_disconnect(struct usb_interface *intf)
{
	struct pwc_device *pdev;
	int hint;

	lock_kernel();
	pdev = usb_get_intfdata (intf);
	usb_set_intfdata (intf, NULL);
	if (pdev == NULL) {
		PWC_ERROR("pwc_disconnect() Called without private pointer.\n");
		goto disconnect_out;
	}
	if (pdev->udev == NULL) {
		PWC_ERROR("pwc_disconnect() already called for %p\n", pdev);
		goto disconnect_out;
	}
	if (pdev->udev != interface_to_usbdev(intf)) {
		PWC_ERROR("pwc_disconnect() Woops: pointer mismatch udev/pdev.\n");
		goto disconnect_out;
	}

	/* We got unplugged; this is signalled by an EPIPE error code */
	if (pdev->vopen) {
		PWC_INFO("Disconnected while webcam is in use!\n");
		pdev->error_status = EPIPE;
	}

	/* Alert waiting processes */
	wake_up_interruptible(&pdev->frameq);
	/* Wait until device is closed */
	while (pdev->vopen)
		schedule();
	/* Device is now closed, so we can safely unregister it */
	PWC_DEBUG_PROBE("Unregistering video device in disconnect().\n");
	pwc_remove_sysfs_files(pdev->vdev);
	video_unregister_device(pdev->vdev);

	/* Free memory (don't set pdev to 0 just yet) */
	kfree(pdev);

disconnect_out:
	/* search device_hint[] table if we occupy a slot, by any chance */
	for (hint = 0; hint < MAX_DEV_HINTS; hint++)
		if (device_hint[hint].pdev == pdev)
			device_hint[hint].pdev = NULL;

	unlock_kernel();
}