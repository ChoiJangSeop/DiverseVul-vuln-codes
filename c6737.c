static int cdrom_ioctl_select_disc(struct cdrom_device_info *cdi,
		unsigned long arg)
{
	cd_dbg(CD_DO_IOCTL, "entering CDROM_SELECT_DISC\n");

	if (!CDROM_CAN(CDC_SELECT_DISC))
		return -ENOSYS;

	if (arg != CDSL_CURRENT && arg != CDSL_NONE) {
		if ((int)arg >= cdi->capacity)
			return -EINVAL;
	}

	/*
	 * ->select_disc is a hook to allow a driver-specific way of
	 * seleting disc.  However, since there is no equivalent hook for
	 * cdrom_slot_status this may not actually be useful...
	 */
	if (cdi->ops->select_disc)
		return cdi->ops->select_disc(cdi, arg);

	cd_dbg(CD_CHANGER, "Using generic cdrom_select_disc()\n");
	return cdrom_select_disc(cdi, arg);
}