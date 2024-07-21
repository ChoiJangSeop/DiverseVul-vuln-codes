static int nft_setelem_parse_data(struct nft_ctx *ctx, struct nft_set *set,
				  struct nft_data_desc *desc,
				  struct nft_data *data,
				  struct nlattr *attr)
{
	int err;

	err = nft_data_init(ctx, data, NFT_DATA_VALUE_MAXLEN, desc, attr);
	if (err < 0)
		return err;

	if (desc->type != NFT_DATA_VERDICT && desc->len != set->dlen) {
		nft_data_release(data, desc->type);
		return -EINVAL;
	}

	return 0;
}