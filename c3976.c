static __be32 nfsacld_proc_setacl(struct svc_rqst * rqstp,
		struct nfsd3_setaclargs *argp,
		struct nfsd_attrstat *resp)
{
	struct inode *inode;
	svc_fh *fh;
	__be32 nfserr = 0;
	int error;

	dprintk("nfsd: SETACL(2acl)   %s\n", SVCFH_fmt(&argp->fh));

	fh = fh_copy(&resp->fh, &argp->fh);
	nfserr = fh_verify(rqstp, &resp->fh, 0, NFSD_MAY_SATTR);
	if (nfserr)
		goto out;

	inode = d_inode(fh->fh_dentry);
	if (!IS_POSIXACL(inode) || !inode->i_op->set_acl) {
		error = -EOPNOTSUPP;
		goto out_errno;
	}

	error = fh_want_write(fh);
	if (error)
		goto out_errno;

	error = inode->i_op->set_acl(inode, argp->acl_access, ACL_TYPE_ACCESS);
	if (error)
		goto out_drop_write;
	error = inode->i_op->set_acl(inode, argp->acl_default,
				     ACL_TYPE_DEFAULT);
	if (error)
		goto out_drop_write;

	fh_drop_write(fh);

	nfserr = fh_getattr(fh, &resp->stat);

out:
	/* argp->acl_{access,default} may have been allocated in
	   nfssvc_decode_setaclargs. */
	posix_acl_release(argp->acl_access);
	posix_acl_release(argp->acl_default);
	return nfserr;
out_drop_write:
	fh_drop_write(fh);
out_errno:
	nfserr = nfserrno(error);
	goto out;
}