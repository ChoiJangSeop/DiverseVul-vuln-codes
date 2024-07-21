int udev_monitor_send_device(struct udev_monitor *udev_monitor, struct udev_device *udev_device)
{
	const char *buf;
	ssize_t len;
	ssize_t count;

	len = udev_device_get_properties_monitor_buf(udev_device, &buf);
	if (len < 32)
		return -1;
	if (udev_monitor->sun.sun_family != 0) {
		count = sendto(udev_monitor->sock,
			       buf, len, 0,
			       (struct sockaddr *)&udev_monitor->sun,
			       udev_monitor->addrlen);
	} else {
		/* no destination besides the muticast group, we will always get -1 ECONNREFUSED */
		count = sendto(udev_monitor->sock,
			       buf, len, 0,
			       (struct sockaddr *)&udev_monitor->snl_peer,
			       sizeof(struct sockaddr_nl));
	}
	info(udev_monitor->udev, "passed %zi bytes to monitor %p, \n", count, udev_monitor);
	return count;
}