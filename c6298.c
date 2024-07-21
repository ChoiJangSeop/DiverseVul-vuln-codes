static ssize_t rebind_store(struct device_driver *dev, const char *buf,
				 size_t count)
{
	int ret;
	int len;
	struct bus_id_priv *bid;

	/* buf length should be less that BUSID_SIZE */
	len = strnlen(buf, BUSID_SIZE);

	if (!(len < BUSID_SIZE))
		return -EINVAL;

	bid = get_busid_priv(buf);
	if (!bid)
		return -ENODEV;

	/* mark the device for deletion so probe ignores it during rescan */
	bid->status = STUB_BUSID_OTHER;

	ret = do_rebind((char *) buf, bid);
	if (ret < 0)
		return ret;

	/* delete device from busid_table */
	del_match_busid((char *) buf);

	return count;
}