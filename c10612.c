int xt_compat_target(void *target, void **dstptr, int *size, int convert)
{
	struct xt_target *t;
	struct compat_xt_entry_target *pcompat;
	struct xt_entry_target *pt;
	u_int16_t tsize;
	int off, ret;

	ret = 0;
	t = ((struct xt_entry_target *)target)->u.kernel.target;
	off = XT_ALIGN(t->targetsize) - COMPAT_XT_ALIGN(t->targetsize);
	switch (convert) {
		case COMPAT_TO_USER:
			pt = (struct xt_entry_target *)target;
			tsize = pt->u.user.target_size;
			if (copy_to_user(*dstptr, pt, tsize)) {
				ret = -EFAULT;
				break;
			}
			tsize -= off;
			if (put_user(tsize, (u_int16_t *)*dstptr))
				ret = -EFAULT;
			*size -= off;
			*dstptr += tsize;
			break;
		case COMPAT_FROM_USER:
			pcompat = (struct compat_xt_entry_target *)target;
			pt = (struct xt_entry_target *)*dstptr;
			tsize = pcompat->u.user.target_size;
			memcpy(pt, pcompat, tsize);
			tsize += off;
			pt->u.user.target_size = tsize;
			*size += off;
			*dstptr += tsize;
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