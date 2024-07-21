int nft_parse_register_load(const struct nlattr *attr, u8 *sreg, u32 len)
{
	u32 reg;
	int err;

	reg = nft_parse_register(attr);
	err = nft_validate_register_load(reg, len);
	if (err < 0)
		return err;

	*sreg = reg;
	return 0;
}