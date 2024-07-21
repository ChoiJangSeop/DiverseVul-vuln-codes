static int fq_init(struct fq *fq, int flows_cnt)
{
	int i;

	memset(fq, 0, sizeof(fq[0]));
	INIT_LIST_HEAD(&fq->backlogs);
	spin_lock_init(&fq->lock);
	fq->flows_cnt = max_t(u32, flows_cnt, 1);
	fq->perturbation = prandom_u32();
	fq->quantum = 300;
	fq->limit = 8192;
	fq->memory_limit = 16 << 20; /* 16 MBytes */

	fq->flows = kcalloc(fq->flows_cnt, sizeof(fq->flows[0]), GFP_KERNEL);
	if (!fq->flows)
		return -ENOMEM;

	for (i = 0; i < fq->flows_cnt; i++)
		fq_flow_init(&fq->flows[i]);

	return 0;
}