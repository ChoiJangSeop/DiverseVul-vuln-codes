struct nfs_server *nfs_clone_server(struct nfs_server *source,
				    struct nfs_fh *fh,
				    struct nfs_fattr *fattr)
{
	struct nfs_server *server;
	struct nfs_fattr fattr_fsinfo;
	int error;

	dprintk("--> nfs_clone_server(,%llx:%llx,)\n",
		(unsigned long long) fattr->fsid.major,
		(unsigned long long) fattr->fsid.minor);

	server = nfs_alloc_server();
	if (!server)
		return ERR_PTR(-ENOMEM);

	/* Copy data from the source */
	server->nfs_client = source->nfs_client;
	atomic_inc(&server->nfs_client->cl_count);
	nfs_server_copy_userdata(server, source);

	server->fsid = fattr->fsid;

	error = nfs_init_server_rpcclient(server, source->client->cl_auth->au_flavor);
	if (error < 0)
		goto out_free_server;
	if (!IS_ERR(source->client_acl))
		nfs_init_server_aclclient(server);

	/* probe the filesystem info for this server filesystem */
	error = nfs_probe_fsinfo(server, fh, &fattr_fsinfo);
	if (error < 0)
		goto out_free_server;

	dprintk("Cloned FSID: %llx:%llx\n",
		(unsigned long long) server->fsid.major,
		(unsigned long long) server->fsid.minor);

	error = nfs_start_lockd(server);
	if (error < 0)
		goto out_free_server;

	spin_lock(&nfs_client_lock);
	list_add_tail(&server->client_link, &server->nfs_client->cl_superblocks);
	list_add_tail(&server->master_link, &nfs_volume_list);
	spin_unlock(&nfs_client_lock);

	server->mount_time = jiffies;

	dprintk("<-- nfs_clone_server() = %p\n", server);
	return server;

out_free_server:
	nfs_free_server(server);
	dprintk("<-- nfs_clone_server() = error %d\n", error);
	return ERR_PTR(error);
}