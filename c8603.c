__u32 skb_get_hash_perturb(const struct sk_buff *skb, u32 perturb)
{
	struct flow_keys keys;

	return ___skb_get_hash(skb, &keys, perturb);
}