static int t220_frontend_attach(struct dvb_usb_adapter *d)
{
	u8 obuf[3] = { 0xe, 0x87, 0 };
	u8 ibuf[] = { 0 };

	if (dvb_usb_generic_rw(d->dev, obuf, 3, ibuf, 1, 0) < 0)
		err("command 0x0e transfer failed.");

	obuf[0] = 0xe;
	obuf[1] = 0x86;
	obuf[2] = 1;

	if (dvb_usb_generic_rw(d->dev, obuf, 3, ibuf, 1, 0) < 0)
		err("command 0x0e transfer failed.");

	obuf[0] = 0xe;
	obuf[1] = 0x80;
	obuf[2] = 0;

	if (dvb_usb_generic_rw(d->dev, obuf, 3, ibuf, 1, 0) < 0)
		err("command 0x0e transfer failed.");

	msleep(50);

	obuf[0] = 0xe;
	obuf[1] = 0x80;
	obuf[2] = 1;

	if (dvb_usb_generic_rw(d->dev, obuf, 3, ibuf, 1, 0) < 0)
		err("command 0x0e transfer failed.");

	obuf[0] = 0x51;

	if (dvb_usb_generic_rw(d->dev, obuf, 1, ibuf, 1, 0) < 0)
		err("command 0x51 transfer failed.");

	d->fe_adap[0].fe = dvb_attach(cxd2820r_attach, &cxd2820r_config,
					&d->dev->i2c_adap, NULL);
	if (d->fe_adap[0].fe != NULL) {
		if (dvb_attach(tda18271_attach, d->fe_adap[0].fe, 0x60,
					&d->dev->i2c_adap, &tda18271_config)) {
			info("Attached TDA18271HD/CXD2820R!");
			return 0;
		}
	}

	info("Failed to attach TDA18271HD/CXD2820R!");
	return -EIO;
}