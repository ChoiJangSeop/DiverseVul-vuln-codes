static void p54u_load_firmware_cb(const struct firmware *firmware,
				  void *context)
{
	struct p54u_priv *priv = context;
	struct usb_device *udev = priv->udev;
	int err;

	complete(&priv->fw_wait_load);
	if (firmware) {
		priv->fw = firmware;
		err = p54u_start_ops(priv);
	} else {
		err = -ENOENT;
		dev_err(&udev->dev, "Firmware not found.\n");
	}

	if (err) {
		struct device *parent = priv->udev->dev.parent;

		dev_err(&udev->dev, "failed to initialize device (%d)\n", err);

		if (parent)
			device_lock(parent);

		device_release_driver(&udev->dev);
		/*
		 * At this point p54u_disconnect has already freed
		 * the "priv" context. Do not use it anymore!
		 */
		priv = NULL;

		if (parent)
			device_unlock(parent);
	}

	usb_put_dev(udev);
}