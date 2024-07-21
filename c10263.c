static int tipc_nl_retrieve_key(struct nlattr **attrs,
				struct tipc_aead_key **key)
{
	struct nlattr *attr = attrs[TIPC_NLA_NODE_KEY];

	if (!attr)
		return -ENODATA;

	*key = (struct tipc_aead_key *)nla_data(attr);
	if (nla_len(attr) < tipc_aead_key_size(*key))
		return -EINVAL;

	return 0;
}