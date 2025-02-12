nfsd4_encode_layoutget(struct nfsd4_compoundres *resp, __be32 nfserr,
		struct nfsd4_layoutget *lgp)
{
	struct xdr_stream *xdr = &resp->xdr;
	const struct nfsd4_layout_ops *ops =
		nfsd4_layout_ops[lgp->lg_layout_type];
	__be32 *p;

	dprintk("%s: err %d\n", __func__, nfserr);
	if (nfserr)
		goto out;

	nfserr = nfserr_resource;
	p = xdr_reserve_space(xdr, 36 + sizeof(stateid_opaque_t));
	if (!p)
		goto out;

	*p++ = cpu_to_be32(1);	/* we always set return-on-close */
	*p++ = cpu_to_be32(lgp->lg_sid.si_generation);
	p = xdr_encode_opaque_fixed(p, &lgp->lg_sid.si_opaque,
				    sizeof(stateid_opaque_t));

	*p++ = cpu_to_be32(1);	/* we always return a single layout */
	p = xdr_encode_hyper(p, lgp->lg_seg.offset);
	p = xdr_encode_hyper(p, lgp->lg_seg.length);
	*p++ = cpu_to_be32(lgp->lg_seg.iomode);
	*p++ = cpu_to_be32(lgp->lg_layout_type);

	nfserr = ops->encode_layoutget(xdr, lgp);
out:
	kfree(lgp->lg_content);
	return nfserr;
}