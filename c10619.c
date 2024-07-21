int xt_compat_match(void *match, void **dstptr, int *size, int convert)
{
	struct xt_match *m;
	struct compat_xt_entry_match *pcompat_m;
	struct xt_entry_match *pm;
	u_int16_t msize;
	int off, ret;

	ret = 0;
	m = ((struct xt_entry_match *)match)->u.kernel.match;
	off = XT_ALIGN(m->matchsize) - COMPAT_XT_ALIGN(m->matchsize);
	switch (convert) {
		case COMPAT_TO_USER:
			pm = (struct xt_entry_match *)match;
			msize = pm->u.user.match_size;
			if (copy_to_user(*dstptr, pm, msize)) {
				ret = -EFAULT;
				break;
			}
			msize -= off;
			if (put_user(msize, (u_int16_t *)*dstptr))
				ret = -EFAULT;
			*size -= off;
			*dstptr += msize;
			break;
		case COMPAT_FROM_USER:
			pcompat_m = (struct compat_xt_entry_match *)match;
			pm = (struct xt_entry_match *)*dstptr;
			msize = pcompat_m->u.user.match_size;
			memcpy(pm, pcompat_m, msize);
			msize += off;
			pm->u.user.match_size = msize;
			*size += off;
			*dstptr += msize;
			break;
		case COMPAT_CALC_SIZE:
			*size += off;
			break;
		default:
			ret = -ENOPROTOOPT;
			break;
	}
	return ret;
}