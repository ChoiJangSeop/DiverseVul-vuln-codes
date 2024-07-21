static int xen_evtchn_cpu_prepare(unsigned int cpu)
{
	int ret = 0;

	if (evtchn_ops->percpu_init)
		ret = evtchn_ops->percpu_init(cpu);

	return ret;
}