static __u8 *cp_report_fixup(struct hid_device *hdev, __u8 *rdesc,
		unsigned int *rsize)
{
	unsigned long quirks = (unsigned long)hid_get_drvdata(hdev);
	unsigned int i;

	if (!(quirks & CP_RDESC_SWAPPED_MIN_MAX))
		return rdesc;

	for (i = 0; i < *rsize - 4; i++)
		if (rdesc[i] == 0x29 && rdesc[i + 2] == 0x19) {
			rdesc[i] = 0x19;
			rdesc[i + 2] = 0x29;
			swap(rdesc[i + 3], rdesc[i + 1]);
		}
	return rdesc;
}