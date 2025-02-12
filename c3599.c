static int cypress_generic_port_probe(struct usb_serial_port *port)
{
	struct usb_serial *serial = port->serial;
	struct cypress_private *priv;

	priv = kzalloc(sizeof(struct cypress_private), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->comm_is_ok = !0;
	spin_lock_init(&priv->lock);
	if (kfifo_alloc(&priv->write_fifo, CYPRESS_BUF_SIZE, GFP_KERNEL)) {
		kfree(priv);
		return -ENOMEM;
	}

	/* Skip reset for FRWD device. It is a workaound:
	   device hangs if it receives SET_CONFIGURE in Configured
	   state. */
	if (!is_frwd(serial->dev))
		usb_reset_configuration(serial->dev);

	priv->cmd_ctrl = 0;
	priv->line_control = 0;
	priv->termios_initialized = 0;
	priv->rx_flags = 0;
	/* Default packet format setting is determined by packet size.
	   Anything with a size larger then 9 must have a separate
	   count field since the 3 bit count field is otherwise too
	   small.  Otherwise we can use the slightly more compact
	   format.  This is in accordance with the cypress_m8 serial
	   converter app note. */
	if (port->interrupt_out_size > 9)
		priv->pkt_fmt = packet_format_1;
	else
		priv->pkt_fmt = packet_format_2;

	if (interval > 0) {
		priv->write_urb_interval = interval;
		priv->read_urb_interval = interval;
		dev_dbg(&port->dev, "%s - read & write intervals forced to %d\n",
			__func__, interval);
	} else {
		priv->write_urb_interval = port->interrupt_out_urb->interval;
		priv->read_urb_interval = port->interrupt_in_urb->interval;
		dev_dbg(&port->dev, "%s - intervals: read=%d write=%d\n",
			__func__, priv->read_urb_interval,
			priv->write_urb_interval);
	}
	usb_set_serial_port_data(port, priv);

	port->port.drain_delay = 256;

	return 0;
}