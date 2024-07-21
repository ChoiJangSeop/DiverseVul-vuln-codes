void __skb_get_hash(struct sk_buff *skb)
{
	struct flow_keys keys;
	u32 hash;

	__flow_hash_secret_init();

	hash = ___skb_get_hash(skb, &keys, hashrnd);

	__skb_set_sw_hash(skb, hash, flow_keys_have_l4(&keys));
}