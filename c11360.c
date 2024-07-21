evdev_log_msg_ratelimit(struct evdev_device *device,
			struct ratelimit *ratelimit,
			enum libinput_log_priority priority,
			const char *format,
			...)
{
	va_list args;
	char buf[1024];

	enum ratelimit_state state;

	if (!is_logged(evdev_libinput_context(device), priority))
		return;

	state = ratelimit_test(ratelimit);
	if (state == RATELIMIT_EXCEEDED)
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

	if (state == RATELIMIT_THRESHOLD) {
		struct human_time ht = to_human_time(ratelimit->interval);
		evdev_log_msg(device,
			      priority,
			      "WARNING: log rate limit exceeded (%d msgs per %d%s). "
			      "Discarding future messages.\n",
			      ratelimit->burst,
			      ht.value,
			      ht.unit);

	}
}