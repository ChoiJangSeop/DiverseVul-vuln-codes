evdev_device_get_sysname(struct evdev_device *device)
{
	return udev_device_get_sysname(device->udev_device);
}