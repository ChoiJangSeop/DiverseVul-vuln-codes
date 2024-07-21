static int do_i2c_loop(struct cmd_tbl *cmdtp, int flag, int argc,
		       char *const argv[])
{
	uint	chip;
	int alen;
	uint	addr;
	uint	length;
	u_char	bytes[16];
	int	delay;
	int ret;
#if CONFIG_IS_ENABLED(DM_I2C)
	struct udevice *dev;
#endif

	if (argc < 3)
		return CMD_RET_USAGE;

	/*
	 * Chip is always specified.
	 */
	chip = hextoul(argv[1], NULL);

	/*
	 * Address is always specified.
	 */
	addr = hextoul(argv[2], NULL);
	alen = get_alen(argv[2], DEFAULT_ADDR_LEN);
	if (alen > 3)
		return CMD_RET_USAGE;
#if CONFIG_IS_ENABLED(DM_I2C)
	ret = i2c_get_cur_bus_chip(chip, &dev);
	if (!ret && alen != -1)
		ret = i2c_set_chip_offset_len(dev, alen);
	if (ret)
		return i2c_report_err(ret, I2C_ERR_WRITE);
#endif

	/*
	 * Length is the number of objects, not number of bytes.
	 */
	length = 1;
	length = hextoul(argv[3], NULL);
	if (length > sizeof(bytes))
		length = sizeof(bytes);

	/*
	 * The delay time (uSec) is optional.
	 */
	delay = 1000;
	if (argc > 3)
		delay = dectoul(argv[4], NULL);
	/*
	 * Run the loop...
	 */
	while (1) {
#if CONFIG_IS_ENABLED(DM_I2C)
		ret = dm_i2c_read(dev, addr, bytes, length);
#else
		ret = i2c_read(chip, addr, alen, bytes, length);
#endif
		if (ret)
			i2c_report_err(ret, I2C_ERR_READ);
		udelay(delay);
	}

	/* NOTREACHED */
	return 0;
}