struct nfs_client *nfs4_alloc_client(const struct nfs_client_initdata *cl_init)
{
	int err;
	struct nfs_client *clp = nfs_alloc_client(cl_init);
	if (IS_ERR(clp))
		return clp;

	err = nfs_get_cb_ident_idr(clp, cl_init->minorversion);
	if (err)
		goto error;

	if (cl_init->minorversion > NFS4_MAX_MINOR_VERSION) {
		err = -EINVAL;
		goto error;
	}

	spin_lock_init(&clp->cl_lock);
	INIT_DELAYED_WORK(&clp->cl_renewd, nfs4_renew_state);
	INIT_LIST_HEAD(&clp->cl_ds_clients);
	rpc_init_wait_queue(&clp->cl_rpcwaitq, "NFS client");
	clp->cl_state = 1 << NFS4CLNT_LEASE_EXPIRED;
	clp->cl_mvops = nfs_v4_minor_ops[cl_init->minorversion];
	clp->cl_mig_gen = 1;
#if IS_ENABLED(CONFIG_NFS_V4_1)
	init_waitqueue_head(&clp->cl_lock_waitq);
#endif
	INIT_LIST_HEAD(&clp->pending_cb_stateids);
	return clp;

error:
	nfs_free_client(clp);
	return ERR_PTR(err);
}