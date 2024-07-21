nfs3svc_decode_symlinkargs(struct svc_rqst *rqstp, __be32 *p,
					struct nfsd3_symlinkargs *args)
{
	unsigned int len, avail;
	char *old, *new;
	struct kvec *vec;

	if (!(p = decode_fh(p, &args->ffh)) ||
	    !(p = decode_filename(p, &args->fname, &args->flen))
		)
		return 0;
	p = decode_sattr3(p, &args->attrs);

	/* now decode the pathname, which might be larger than the first page.
	 * As we have to check for nul's anyway, we copy it into a new page
	 * This page appears in the rq_res.pages list, but as pages_len is always
	 * 0, it won't get in the way
	 */
	len = ntohl(*p++);
	if (len == 0 || len > NFS3_MAXPATHLEN || len >= PAGE_SIZE)
		return 0;
	args->tname = new = page_address(*(rqstp->rq_next_page++));
	args->tlen = len;
	/* first copy and check from the first page */
	old = (char*)p;
	vec = &rqstp->rq_arg.head[0];
	avail = vec->iov_len - (old - (char*)vec->iov_base);
	while (len && avail && *old) {
		*new++ = *old++;
		len--;
		avail--;
	}
	/* now copy next page if there is one */
	if (len && !avail && rqstp->rq_arg.page_len) {
		avail = min_t(unsigned int, rqstp->rq_arg.page_len, PAGE_SIZE);
		old = page_address(rqstp->rq_arg.pages[0]);
	}
	while (len && avail && *old) {
		*new++ = *old++;
		len--;
		avail--;
	}
	*new = '\0';
	if (len)
		return 0;

	return 1;
}