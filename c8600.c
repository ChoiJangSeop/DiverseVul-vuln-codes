u32 __skb_get_hash_symmetric(const struct sk_buff *skb)
{
	struct flow_keys keys;

	__flow_hash_secret_init();

	memset(&keys, 0, sizeof(keys));
	__skb_flow_dissect(NULL, skb, &flow_keys_dissector_symmetric,
			   &keys, NULL, 0, 0, 0,
			   FLOW_DISSECTOR_F_STOP_AT_FLOW_LABEL);

	return __flow_hash_from_keys(&keys, hashrnd);
}