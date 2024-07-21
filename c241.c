static int chip_write_masked(struct CHIPSTATE *chip, int subaddr, int val, int mask)
{
	if (mask != 0) {
		if (-1 == subaddr) {
			val = (chip->shadow.bytes[1] & ~mask) | (val & mask);
		} else {
			val = (chip->shadow.bytes[subaddr+1] & ~mask) | (val & mask);
		}
	}
	return chip_write(chip, subaddr, val);
}