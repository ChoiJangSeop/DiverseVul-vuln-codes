int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor)
{
	int err;
	const int on = 1;

	if (udev_monitor->snl.nl_family != 0) {
		err = bind(udev_monitor->sock,
			   (struct sockaddr *)&udev_monitor->snl, sizeof(struct sockaddr_nl));
		if (err < 0) {
			err(udev_monitor->udev, "bind failed: %m\n");
			return err;
		}
		dbg(udev_monitor->udev, "monitor %p listening on netlink\n", udev_monitor);
	} else if (udev_monitor->sun.sun_family != 0) {
		err = bind(udev_monitor->sock,
			   (struct sockaddr *)&udev_monitor->sun, udev_monitor->addrlen);
		if (err < 0) {
			err(udev_monitor->udev, "bind failed: %m\n");
			return err;
		}
		/* enable receiving of the sender credentials */
		setsockopt(udev_monitor->sock, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));
		dbg(udev_monitor->udev, "monitor %p listening on socket\n", udev_monitor);
	}
	return 0;
}