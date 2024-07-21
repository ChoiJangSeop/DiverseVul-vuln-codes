static int adis_update_scan_mode_burst(struct iio_dev *indio_dev,
	const unsigned long *scan_mask)
{
	struct adis *adis = iio_device_get_drvdata(indio_dev);
	unsigned int burst_length;
	u8 *tx;

	/* All but the timestamp channel */
	burst_length = (indio_dev->num_channels - 1) * sizeof(u16);
	burst_length += adis->burst->extra_len;

	adis->xfer = kcalloc(2, sizeof(*adis->xfer), GFP_KERNEL);
	if (!adis->xfer)
		return -ENOMEM;

	adis->buffer = kzalloc(burst_length + sizeof(u16), GFP_KERNEL);
	if (!adis->buffer)
		return -ENOMEM;

	tx = adis->buffer + burst_length;
	tx[0] = ADIS_READ_REG(adis->burst->reg_cmd);
	tx[1] = 0;

	adis->xfer[0].tx_buf = tx;
	adis->xfer[0].bits_per_word = 8;
	adis->xfer[0].len = 2;
	adis->xfer[1].rx_buf = adis->buffer;
	adis->xfer[1].bits_per_word = 8;
	adis->xfer[1].len = burst_length;

	spi_message_init(&adis->msg);
	spi_message_add_tail(&adis->xfer[0], &adis->msg);
	spi_message_add_tail(&adis->xfer[1], &adis->msg);

	return 0;
}