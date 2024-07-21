nfsd4_verify_copy(struct svc_rqst *rqstp, struct nfsd4_compound_state *cstate,
		  stateid_t *src_stateid, struct file **src,
		  stateid_t *dst_stateid, struct file **dst)
{
	__be32 status;

	status = nfs4_preprocess_stateid_op(rqstp, cstate, &cstate->save_fh,
					    src_stateid, RD_STATE, src, NULL);
	if (status) {
		dprintk("NFSD: %s: couldn't process src stateid!\n", __func__);
		goto out;
	}

	status = nfs4_preprocess_stateid_op(rqstp, cstate, &cstate->current_fh,
					    dst_stateid, WR_STATE, dst, NULL);
	if (status) {
		dprintk("NFSD: %s: couldn't process dst stateid!\n", __func__);
		goto out_put_src;
	}

	/* fix up for NFS-specific error code */
	if (!S_ISREG(file_inode(*src)->i_mode) ||
	    !S_ISREG(file_inode(*dst)->i_mode)) {
		status = nfserr_wrong_type;
		goto out_put_dst;
	}

out:
	return status;
out_put_dst:
	fput(*dst);
out_put_src:
	fput(*src);
	goto out;
}