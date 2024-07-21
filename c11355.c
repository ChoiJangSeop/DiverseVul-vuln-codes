evdev_device_create(struct libinput_seat *seat,
		    struct udev_device *udev_device)
{
	struct libinput *libinput = seat->libinput;
	struct evdev_device *device = NULL;
	int rc;
	int fd;
	int unhandled_device = 0;
	const char *devnode = udev_device_get_devnode(udev_device);
	const char *sysname = udev_device_get_sysname(udev_device);

	if (!devnode) {
		log_info(libinput, "%s: no device node associated\n", sysname);
		return NULL;
	}

	if (udev_device_should_be_ignored(udev_device)) {
		log_debug(libinput, "%s: device is ignored\n", sysname);
		return NULL;
	}

	/* Use non-blocking mode so that we can loop on read on
	 * evdev_device_data() until all events on the fd are
	 * read.  mtdev_get() also expects this. */
	fd = open_restricted(libinput, devnode,
			     O_RDWR | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) {
		log_info(libinput,
			 "%s: opening input device '%s' failed (%s).\n",
			 sysname,
			 devnode,
			 strerror(-fd));
		return NULL;
	}

	if (!evdev_device_have_same_syspath(udev_device, fd))
		goto err;

	device = zalloc(sizeof *device);

	libinput_device_init(&device->base, seat);
	libinput_seat_ref(seat);

	evdev_drain_fd(fd);

	rc = libevdev_new_from_fd(fd, &device->evdev);
	if (rc != 0)
		goto err;

	libevdev_set_clock_id(device->evdev, CLOCK_MONOTONIC);
	libevdev_set_device_log_function(device->evdev,
					 libevdev_log_func,
					 LIBEVDEV_LOG_ERROR,
					 libinput);
	device->seat_caps = 0;
	device->is_mt = 0;
	device->mtdev = NULL;
	device->udev_device = udev_device_ref(udev_device);
	device->dispatch = NULL;
	device->fd = fd;
	device->devname = libevdev_get_name(device->evdev);
	device->scroll.threshold = 5.0; /* Default may be overridden */
	device->scroll.direction_lock_threshold = 5.0; /* Default may be overridden */
	device->scroll.direction = 0;
	device->scroll.wheel_click_angle =
		evdev_read_wheel_click_props(device);
	device->model_flags = evdev_read_model_flags(device);
	device->dpi = DEFAULT_MOUSE_DPI;

	/* at most 5 SYN_DROPPED log-messages per 30s */
	ratelimit_init(&device->syn_drop_limit, s2us(30), 5);
	/* at most 5 "delayed processing" log messages per hour */
	ratelimit_init(&device->delay_warning_limit, s2us(60 * 60), 5);
	/* at most 5 log-messages per 5s */
	ratelimit_init(&device->nonpointer_rel_limit, s2us(5), 5);

	matrix_init_identity(&device->abs.calibration);
	matrix_init_identity(&device->abs.usermatrix);
	matrix_init_identity(&device->abs.default_calibration);

	evdev_pre_configure_model_quirks(device);

	device->dispatch = evdev_configure_device(device);
	if (device->dispatch == NULL || device->seat_caps == 0)
		goto err;

	device->source =
		libinput_add_fd(libinput, fd, evdev_device_dispatch, device);
	if (!device->source)
		goto err;

	if (!evdev_set_device_group(device, udev_device))
		goto err;

	list_insert(seat->devices_list.prev, &device->base.link);

	evdev_notify_added_device(device);

	return device;

err:
	close_restricted(libinput, fd);
	if (device) {
		unhandled_device = device->seat_caps == 0;
		evdev_device_destroy(device);
	}

	return unhandled_device ? EVDEV_UNHANDLED_DEVICE :  NULL;
}