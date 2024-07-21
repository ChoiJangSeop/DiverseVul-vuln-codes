static ssize_t nports_show(struct device *dev, struct device_attribute *attr,
			   char *out)
{
	char *s = out;

	/*
	 * Half the ports are for SPEED_HIGH and half for SPEED_SUPER, thus the * 2.
	 */
	out += sprintf(out, "%d\n", VHCI_PORTS * vhci_num_controllers);
	return out - s;
}