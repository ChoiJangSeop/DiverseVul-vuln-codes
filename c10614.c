static int compat_ipt_standard_fn(void *target,
		void **dstptr, int *size, int convert)
{
	struct compat_ipt_standard_target compat_st, *pcompat_st;
	struct ipt_standard_target st, *pst;
	int ret;

	ret = 0;
	switch (convert) {
		case COMPAT_TO_USER:
			pst = target;
			memcpy(&compat_st.target, &pst->target,
				sizeof(compat_st.target));
			compat_st.verdict = pst->verdict;
			if (compat_st.verdict > 0)
				compat_st.verdict -=
					compat_calc_jump(compat_st.verdict);
			compat_st.target.u.user.target_size = IPT_ST_COMPAT_LEN;
			if (copy_to_user(*dstptr, &compat_st, IPT_ST_COMPAT_LEN))
				ret = -EFAULT;
			*size -= IPT_ST_OFFSET;
			*dstptr += IPT_ST_COMPAT_LEN;
			break;
		case COMPAT_FROM_USER:
			pcompat_st = target;
			memcpy(&st.target, &pcompat_st->target, IPT_ST_COMPAT_LEN);
			st.verdict = pcompat_st->verdict;
			if (st.verdict > 0)
				st.verdict += compat_calc_jump(st.verdict);
			st.target.u.user.target_size = IPT_ST_LEN;
			memcpy(*dstptr, &st, IPT_ST_LEN);
			*size += IPT_ST_OFFSET;
			*dstptr += IPT_ST_LEN;
			break;
		case COMPAT_CALC_SIZE:
			*size += IPT_ST_OFFSET;
			break;
		default:
			ret = -ENOPROTOOPT;
			break;
	}
	return ret;
}