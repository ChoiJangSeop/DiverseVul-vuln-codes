static int ax25_ctl_ioctl(const unsigned int cmd, void __user *arg)
{
	struct ax25_ctl_struct ax25_ctl;
	ax25_digi digi;
	ax25_dev *ax25_dev;
	ax25_cb *ax25;
	unsigned int k;
	int ret = 0;

	if (copy_from_user(&ax25_ctl, arg, sizeof(ax25_ctl)))
		return -EFAULT;

	if ((ax25_dev = ax25_addr_ax25dev(&ax25_ctl.port_addr)) == NULL)
		return -ENODEV;

	if (ax25_ctl.digi_count > AX25_MAX_DIGIS)
		return -EINVAL;

	if (ax25_ctl.arg > ULONG_MAX / HZ && ax25_ctl.cmd != AX25_KILL)
		return -EINVAL;

	digi.ndigi = ax25_ctl.digi_count;
	for (k = 0; k < digi.ndigi; k++)
		digi.calls[k] = ax25_ctl.digi_addr[k];

	if ((ax25 = ax25_find_cb(&ax25_ctl.source_addr, &ax25_ctl.dest_addr, &digi, ax25_dev->dev)) == NULL)
		return -ENOTCONN;

	switch (ax25_ctl.cmd) {
	case AX25_KILL:
		ax25_send_control(ax25, AX25_DISC, AX25_POLLON, AX25_COMMAND);
#ifdef CONFIG_AX25_DAMA_SLAVE
		if (ax25_dev->dama.slave && ax25->ax25_dev->values[AX25_VALUES_PROTOCOL] == AX25_PROTO_DAMA_SLAVE)
			ax25_dama_off(ax25);
#endif
		ax25_disconnect(ax25, ENETRESET);
		break;

	case AX25_WINDOW:
		if (ax25->modulus == AX25_MODULUS) {
			if (ax25_ctl.arg < 1 || ax25_ctl.arg > 7)
				goto einval_put;
		} else {
			if (ax25_ctl.arg < 1 || ax25_ctl.arg > 63)
				goto einval_put;
		}
		ax25->window = ax25_ctl.arg;
		break;

	case AX25_T1:
		if (ax25_ctl.arg < 1 || ax25_ctl.arg > ULONG_MAX / HZ)
			goto einval_put;
		ax25->rtt = (ax25_ctl.arg * HZ) / 2;
		ax25->t1  = ax25_ctl.arg * HZ;
		break;

	case AX25_T2:
		if (ax25_ctl.arg < 1 || ax25_ctl.arg > ULONG_MAX / HZ)
			goto einval_put;
		ax25->t2 = ax25_ctl.arg * HZ;
		break;

	case AX25_N2:
		if (ax25_ctl.arg < 1 || ax25_ctl.arg > 31)
			goto einval_put;
		ax25->n2count = 0;
		ax25->n2 = ax25_ctl.arg;
		break;

	case AX25_T3:
		if (ax25_ctl.arg > ULONG_MAX / HZ)
			goto einval_put;
		ax25->t3 = ax25_ctl.arg * HZ;
		break;

	case AX25_IDLE:
		if (ax25_ctl.arg > ULONG_MAX / (60 * HZ))
			goto einval_put;

		ax25->idle = ax25_ctl.arg * 60 * HZ;
		break;

	case AX25_PACLEN:
		if (ax25_ctl.arg < 16 || ax25_ctl.arg > 65535)
			goto einval_put;
		ax25->paclen = ax25_ctl.arg;
		break;

	default:
		goto einval_put;
	  }

out_put:
	ax25_dev_put(ax25_dev);
	ax25_cb_put(ax25);
	return ret;

einval_put:
	ret = -EINVAL;
	goto out_put;
}