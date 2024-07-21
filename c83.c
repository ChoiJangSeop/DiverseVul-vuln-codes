calc_delta_fair(unsigned long delta_exec, struct load_weight *lw)
{
	return calc_delta_mine(delta_exec, NICE_0_LOAD, lw);
}