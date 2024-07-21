static u64 __sched_vslice(unsigned long rq_weight, unsigned long nr_running)
{
	u64 vslice = __sched_period(nr_running);

	vslice *= NICE_0_LOAD;
	do_div(vslice, rq_weight);

	return vslice;
}