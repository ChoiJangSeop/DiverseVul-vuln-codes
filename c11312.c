int ax25_fwd_ioctl(unsigned int cmd, struct ax25_fwd_struct *fwd)
{
	ax25_dev *ax25_dev, *fwd_dev;

	if ((ax25_dev = ax25_addr_ax25dev(&fwd->port_from)) == NULL)
		return -EINVAL;

	switch (cmd) {
	case SIOCAX25ADDFWD:
		if ((fwd_dev = ax25_addr_ax25dev(&fwd->port_to)) == NULL)
			return -EINVAL;
		if (ax25_dev->forward != NULL)
			return -EINVAL;
		ax25_dev->forward = fwd_dev->dev;
		break;

	case SIOCAX25DELFWD:
		if (ax25_dev->forward == NULL)
			return -EINVAL;
		ax25_dev->forward = NULL;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}