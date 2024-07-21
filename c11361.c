evdev_log_msg(struct evdev_device *device,
	      enum libinput_log_priority priority,
	      const char *format,
	      ...)
{
	va_list args;
	char buf[1024];

	if (!is_logged(evdev_libinput_context(device), priority))
		return;

	/* Anything info and above is user-visible, use the device name */
	snprintf(buf,
		 sizeof(buf),
		 "%-7s - %s%s%s",
		 evdev_device_get_sysname(device),
		 (priority > LIBINPUT_LOG_PRIORITY_DEBUG) ?  device->devname : "",
		 (priority > LIBINPUT_LOG_PRIORITY_DEBUG) ?  ": " : "",
		 format);

	va_start(args, format);
	log_msg_va(evdev_libinput_context(device), priority, buf, args);
	va_end(args);

}