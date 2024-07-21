static int nft_set_desc_concat(struct nft_set_desc *desc,
			       const struct nlattr *nla)
{
	struct nlattr *attr;
	int rem, err;

	nla_for_each_nested(attr, nla, rem) {
		if (nla_type(attr) != NFTA_LIST_ELEM)
			return -EINVAL;

		err = nft_set_desc_concat_parse(attr, desc);
		if (err < 0)
			return err;
	}

	return 0;
}