static int hidp_add_connection(struct input_device *idev)
{
	struct hidp_connadd_req *req;
	sdp_record_t *rec;
	char src_addr[18], dst_addr[18];
	char filename[PATH_MAX];
	GKeyFile *key_file;
	char handle[11], *str;
	GError *gerr = NULL;
	int err;

	req = g_new0(struct hidp_connadd_req, 1);
	req->ctrl_sock = g_io_channel_unix_get_fd(idev->ctrl_io);
	req->intr_sock = g_io_channel_unix_get_fd(idev->intr_io);
	req->flags     = 0;
	req->idle_to   = idle_timeout;

	ba2str(&idev->src, src_addr);
	ba2str(&idev->dst, dst_addr);

	snprintf(filename, PATH_MAX, STORAGEDIR "/%s/cache/%s", src_addr,
								dst_addr);
	sprintf(handle, "0x%8.8X", idev->handle);

	key_file = g_key_file_new();
	g_key_file_load_from_file(key_file, filename, 0, NULL);
	str = g_key_file_get_string(key_file, "ServiceRecords", handle, NULL);
	g_key_file_free(key_file);

	if (!str) {
		error("Rejected connection from unknown device %s", dst_addr);
		err = -EPERM;
		goto cleanup;
	}

	rec = record_from_string(str);
	g_free(str);

	err = extract_hid_record(rec, req);
	sdp_record_free(rec);
	if (err < 0) {
		error("Could not parse HID SDP record: %s (%d)", strerror(-err),
									-err);
		goto cleanup;
	}

	req->vendor = btd_device_get_vendor(idev->device);
	req->product = btd_device_get_product(idev->device);
	req->version = btd_device_get_version(idev->device);

	if (device_name_known(idev->device))
		device_get_name(idev->device, req->name, sizeof(req->name));

	/* Encryption is mandatory for keyboards */
	if (req->subclass & 0x40) {
		if (!bt_io_set(idev->intr_io, &gerr,
					BT_IO_OPT_SEC_LEVEL, BT_IO_SEC_MEDIUM,
					BT_IO_OPT_INVALID)) {
			error("btio: %s", gerr->message);
			g_error_free(gerr);
			err = -EFAULT;
			goto cleanup;
		}

		idev->req = req;
		idev->sec_watch = g_io_add_watch(idev->intr_io, G_IO_OUT,
							encrypt_notify, idev);

		return 0;
	}

	if (idev->uhid)
		err = uhid_connadd(idev, req);
	else
		err = ioctl_connadd(req);

cleanup:
	g_free(req->rd_data);
	g_free(req);

	return err;
}