void hid_dump_input(struct hid_device *hdev, struct hid_usage *usage, __s32 value)
{
	char *buf;
	int len;

	buf = hid_resolv_usage(usage->hid, NULL);
	if (!buf)
		return;
	len = strlen(buf);
	snprintf(buf + len, HID_DEBUG_BUFSIZE - len - 1, " = %d\n", value);

	hid_debug_event(hdev, buf);

	kfree(buf);
        wake_up_interruptible(&hdev->debug_wait);

}