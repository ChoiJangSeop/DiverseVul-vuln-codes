static int _nfs4_get_security_label(struct inode *inode, void *buf,
					size_t buflen)
{
	struct nfs_server *server = NFS_SERVER(inode);
	struct nfs_fattr fattr;
	struct nfs4_label label = {0, 0, buflen, buf};

	u32 bitmask[3] = { 0, 0, FATTR4_WORD2_SECURITY_LABEL };
	struct nfs4_getattr_arg arg = {
		.fh		= NFS_FH(inode),
		.bitmask	= bitmask,
	};
	struct nfs4_getattr_res res = {
		.fattr		= &fattr,
		.label		= &label,
		.server		= server,
	};
	struct rpc_message msg = {
		.rpc_proc	= &nfs4_procedures[NFSPROC4_CLNT_GETATTR],
		.rpc_argp	= &arg,
		.rpc_resp	= &res,
	};
	int ret;

	nfs_fattr_init(&fattr);

	ret = nfs4_call_sync(server->client, server, &msg, &arg.seq_args, &res.seq_res, 0);
	if (ret)
		return ret;
	if (!(fattr.valid & NFS_ATTR_FATTR_V4_SECURITY_LABEL))
		return -ENOENT;
	if (buflen < label.len)
		return -ERANGE;
	return 0;
}