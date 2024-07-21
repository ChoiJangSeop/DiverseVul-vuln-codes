static int vt_disallocate(unsigned int vc_num)
{
	struct vc_data *vc = NULL;
	int ret = 0;

	console_lock();
	if (vt_busy(vc_num))
		ret = -EBUSY;
	else if (vc_num)
		vc = vc_deallocate(vc_num);
	console_unlock();

	if (vc && vc_num >= MIN_NR_CONSOLES) {
		tty_port_destroy(&vc->port);
		kfree(vc);
	}

	return ret;
}