h2_vfp_body(struct vfp_ctx *vc, struct vfp_entry *vfe, void *ptr, ssize_t *lp)
{
	struct h2_req *r2;
	struct h2_sess *h2;
	unsigned l;
	enum vfp_status retval = VFP_OK;

	CHECK_OBJ_NOTNULL(vc, VFP_CTX_MAGIC);
	CHECK_OBJ_NOTNULL(vfe, VFP_ENTRY_MAGIC);
	CAST_OBJ_NOTNULL(r2, vfe->priv1, H2_REQ_MAGIC);
	h2 = r2->h2sess;

	AN(ptr);
	AN(lp);
	l = *lp;
	*lp = 0;

	Lck_Lock(&h2->sess->mtx);
	assert (r2->state == H2_S_OPEN);
	r2->cond = &vc->wrk->cond;
	while (h2->mailcall != r2 && h2->error == 0 && r2->error == 0)
		AZ(Lck_CondWait(r2->cond, &h2->sess->mtx, 0));
	r2->cond = NULL;
	if (h2->error || r2->error) {
		retval = VFP_ERROR;
	} else {
		assert(h2->mailcall == r2);
		if (l > h2->rxf_len)
			l = h2->rxf_len;
		if (l > 0) {
			memcpy(ptr, h2->rxf_data, l);
			h2->rxf_data += l;
			h2->rxf_len -= l;
		}
		*lp = l;
		if (h2->rxf_len > 0) {
			/* We ran out of storage: Have VFP call us
			 * again with a fresh buffer */
			Lck_Unlock(&h2->sess->mtx);
			return (VFP_OK);
		}
		if (h2->rxf_len == 0) {
			if (h2->rxf_flags & H2FF_DATA_END_STREAM) {
				retval = VFP_END;
				r2->state = H2_S_CLOS_REM;
			}
		}
		h2->mailcall = NULL;
		AZ(pthread_cond_signal(h2->cond));
	}
	Lck_Unlock(&h2->sess->mtx);
	return (retval);
}