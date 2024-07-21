static ssize_t state_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct memory_block *mem = to_memory_block(dev);
	ssize_t len = 0;

	/*
	 * We can probably put these states in a nice little array
	 * so that they're not open-coded
	 */
	switch (mem->state) {
	case MEM_ONLINE:
		len = sprintf(buf, "online\n");
		break;
	case MEM_OFFLINE:
		len = sprintf(buf, "offline\n");
		break;
	case MEM_GOING_OFFLINE:
		len = sprintf(buf, "going-offline\n");
		break;
	default:
		len = sprintf(buf, "ERROR-UNKNOWN-%ld\n",
				mem->state);
		WARN_ON(1);
		break;
	}

	return len;
}