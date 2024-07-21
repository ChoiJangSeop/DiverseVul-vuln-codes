static void vt_disallocate_all(void)
{
	struct vc_data *vc[MAX_NR_CONSOLES];
	int i;

	console_lock();
	for (i = 1; i < MAX_NR_CONSOLES; i++)
		if (!vt_busy(i))
			vc[i] = vc_deallocate(i);
		else
			vc[i] = NULL;
	console_unlock();

	for (i = 1; i < MAX_NR_CONSOLES; i++) {
		if (vc[i] && i >= MIN_NR_CONSOLES) {
			tty_port_destroy(&vc[i]->port);
			kfree(vc[i]);
		}
	}
}