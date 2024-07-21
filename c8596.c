static u32 fq_flow_idx(struct fq *fq, struct sk_buff *skb)
{
	u32 hash = skb_get_hash_perturb(skb, fq->perturbation);

	return reciprocal_scale(hash, fq->flows_cnt);
}