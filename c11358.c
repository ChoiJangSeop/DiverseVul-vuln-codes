evdev_device_destroy(struct evdev_device *device)
{
	struct evdev_dispatch *dispatch;

	dispatch = device->dispatch;
	if (dispatch)
		dispatch->interface->destroy(dispatch);

	if (device->base.group)
		libinput_device_group_unref(device->base.group);

	free(device->output_name);
	filter_destroy(device->pointer.filter);
	libinput_timer_destroy(&device->scroll.timer);
	libinput_timer_destroy(&device->middlebutton.timer);
	libinput_seat_unref(device->base.seat);
	libevdev_free(device->evdev);
	udev_device_unref(device->udev_device);
	free(device);
}