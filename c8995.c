target_count_increase(struct iter_qstate* iq, int num)
{
	target_count_create(iq);
	if(iq->target_count)
		iq->target_count[1] += num;
}