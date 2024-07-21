int nft_parse_register_store(const struct nft_ctx *ctx,
			     const struct nlattr *attr, u8 *dreg,
			     const struct nft_data *data,
			     enum nft_data_types type, unsigned int len)
{
	int err;
	u32 reg;

	reg = nft_parse_register(attr);
	err = nft_validate_register_store(ctx, reg, data, type, len);
	if (err < 0)
		return err;

	*dreg = reg;
	return 0;
}