static void sfb_init_perturbation(u32 slot, struct sfb_sched_data *q)
{
	q->bins[slot].perturbation = prandom_u32();
}