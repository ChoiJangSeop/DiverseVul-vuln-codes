static void xen_irq_lateeoi_locked(struct irq_info *info)
{
	evtchn_port_t evtchn;

	evtchn = info->evtchn;
	if (!VALID_EVTCHN(evtchn))
		return;

	unmask_evtchn(evtchn);
}