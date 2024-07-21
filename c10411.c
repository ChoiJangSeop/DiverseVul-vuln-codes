static void scalar32_min_max_and(struct bpf_reg_state *dst_reg,
				 struct bpf_reg_state *src_reg)
{
	bool src_known = tnum_subreg_is_const(src_reg->var_off);
	bool dst_known = tnum_subreg_is_const(dst_reg->var_off);
	struct tnum var32_off = tnum_subreg(dst_reg->var_off);
	s32 smin_val = src_reg->s32_min_value;
	u32 umax_val = src_reg->u32_max_value;

	/* Assuming scalar64_min_max_and will be called so its safe
	 * to skip updating register for known 32-bit case.
	 */
	if (src_known && dst_known)
		return;

	/* We get our minimum from the var_off, since that's inherently
	 * bitwise.  Our maximum is the minimum of the operands' maxima.
	 */
	dst_reg->u32_min_value = var32_off.value;
	dst_reg->u32_max_value = min(dst_reg->u32_max_value, umax_val);
	if (dst_reg->s32_min_value < 0 || smin_val < 0) {
		/* Lose signed bounds when ANDing negative numbers,
		 * ain't nobody got time for that.
		 */
		dst_reg->s32_min_value = S32_MIN;
		dst_reg->s32_max_value = S32_MAX;
	} else {
		/* ANDing two positives gives a positive, so safe to
		 * cast result into s64.
		 */
		dst_reg->s32_min_value = dst_reg->u32_min_value;
		dst_reg->s32_max_value = dst_reg->u32_max_value;
	}

}