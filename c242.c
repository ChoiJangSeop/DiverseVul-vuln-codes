static int chip_write(struct CHIPSTATE *chip, int subaddr, int val)
{
	unsigned char buffer[2];

	if (-1 == subaddr) {
		v4l_dbg(1, debug, chip->c, "%s: chip_write: 0x%x\n",
			chip->c->name, val);
		chip->shadow.bytes[1] = val;
		buffer[0] = val;
		if (1 != i2c_master_send(chip->c,buffer,1)) {
			v4l_warn(chip->c, "%s: I/O error (write 0x%x)\n",
				chip->c->name, val);
			return -1;
		}
	} else {
		v4l_dbg(1, debug, chip->c, "%s: chip_write: reg%d=0x%x\n",
			chip->c->name, subaddr, val);
		chip->shadow.bytes[subaddr+1] = val;
		buffer[0] = subaddr;
		buffer[1] = val;
		if (2 != i2c_master_send(chip->c,buffer,2)) {
			v4l_warn(chip->c, "%s: I/O error (write reg%d=0x%x)\n",
			chip->c->name, subaddr, val);
			return -1;
		}
	}
	return 0;
}