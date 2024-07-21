target_count_create(struct iter_qstate* iq)
{
	if(!iq->target_count) {
		iq->target_count = (int*)calloc(2, sizeof(int));
		/* if calloc fails we simply do not track this number */
		if(iq->target_count)
			iq->target_count[0] = 1;
	}
}