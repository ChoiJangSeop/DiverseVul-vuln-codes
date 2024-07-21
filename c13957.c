static void coerce_reg_to_32(struct bpf_reg_state *reg)
{
	/* clear high 32 bits */
	reg->var_off = tnum_cast(reg->var_off, 4);
	/* Update bounds */
	__update_reg_bounds(reg);
}