static int usb_console_setup(struct console *co, char *options)
{
	struct usbcons_info *info = &usbcons_info;
	int baud = 9600;
	int bits = 8;
	int parity = 'n';
	int doflow = 0;
	int cflag = CREAD | HUPCL | CLOCAL;
	char *s;
	struct usb_serial *serial;
	struct usb_serial_port *port;
	int retval;
	struct tty_struct *tty = NULL;
	struct ktermios dummy;

	if (options) {
		baud = simple_strtoul(options, NULL, 10);
		s = options;
		while (*s >= '0' && *s <= '9')
			s++;
		if (*s)
			parity = *s++;
		if (*s)
			bits   = *s++ - '0';
		if (*s)
			doflow = (*s++ == 'r');
	}
	
	/* Sane default */
	if (baud == 0)
		baud = 9600;

	switch (bits) {
	case 7:
		cflag |= CS7;
		break;
	default:
	case 8:
		cflag |= CS8;
		break;
	}
	switch (parity) {
	case 'o': case 'O':
		cflag |= PARODD;
		break;
	case 'e': case 'E':
		cflag |= PARENB;
		break;
	}
	co->cflag = cflag;

	/*
	 * no need to check the index here: if the index is wrong, console
	 * code won't call us
	 */
	port = usb_serial_port_get_by_minor(co->index);
	if (port == NULL) {
		/* no device is connected yet, sorry :( */
		pr_err("No USB device connected to ttyUSB%i\n", co->index);
		return -ENODEV;
	}
	serial = port->serial;

	retval = usb_autopm_get_interface(serial->interface);
	if (retval)
		goto error_get_interface;

	tty_port_tty_set(&port->port, NULL);

	info->port = port;

	++port->port.count;
	if (!tty_port_initialized(&port->port)) {
		if (serial->type->set_termios) {
			/*
			 * allocate a fake tty so the driver can initialize
			 * the termios structure, then later call set_termios to
			 * configure according to command line arguments
			 */
			tty = kzalloc(sizeof(*tty), GFP_KERNEL);
			if (!tty) {
				retval = -ENOMEM;
				goto reset_open_count;
			}
			kref_init(&tty->kref);
			tty->driver = usb_serial_tty_driver;
			tty->index = co->index;
			init_ldsem(&tty->ldisc_sem);
			spin_lock_init(&tty->files_lock);
			INIT_LIST_HEAD(&tty->tty_files);
			kref_get(&tty->driver->kref);
			__module_get(tty->driver->owner);
			tty->ops = &usb_console_fake_tty_ops;
			tty_init_termios(tty);
			tty_port_tty_set(&port->port, tty);
		}

		/* only call the device specific open if this
		 * is the first time the port is opened */
		retval = serial->type->open(NULL, port);
		if (retval) {
			dev_err(&port->dev, "could not open USB console port\n");
			goto fail;
		}

		if (serial->type->set_termios) {
			tty->termios.c_cflag = cflag;
			tty_termios_encode_baud_rate(&tty->termios, baud, baud);
			memset(&dummy, 0, sizeof(struct ktermios));
			serial->type->set_termios(tty, port, &dummy);

			tty_port_tty_set(&port->port, NULL);
			tty_kref_put(tty);
		}
		tty_port_set_initialized(&port->port, 1);
	}
	/* Now that any required fake tty operations are completed restore
	 * the tty port count */
	--port->port.count;
	/* The console is special in terms of closing the device so
	 * indicate this port is now acting as a system console. */
	port->port.console = 1;

	mutex_unlock(&serial->disc_mutex);
	return retval;

 fail:
	tty_port_tty_set(&port->port, NULL);
	tty_kref_put(tty);
 reset_open_count:
	port->port.count = 0;
	usb_autopm_put_interface(serial->interface);
 error_get_interface:
	usb_serial_put(serial);
	mutex_unlock(&serial->disc_mutex);
	return retval;
}