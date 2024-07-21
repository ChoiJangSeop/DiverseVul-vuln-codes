int nfs4_path_walk(struct nfs_server *server,
		   struct nfs_fh *mntfh,
		   const char *path)
{
	struct nfs_fsinfo fsinfo;
	struct nfs_fattr fattr;
	struct nfs_fh lastfh;
	struct qstr name;
	int ret;

	dprintk("--> nfs4_path_walk(,,%s)\n", path);

	fsinfo.fattr = &fattr;
	nfs_fattr_init(&fattr);

	/* Eat leading slashes */
	while (*path == '/')
		path++;

	/* Start by getting the root filehandle from the server */
	ret = server->nfs_client->rpc_ops->getroot(server, mntfh, &fsinfo);
	if (ret < 0) {
		dprintk("nfs4_get_root: getroot error = %d\n", -ret);
		return ret;
	}

	if (fattr.type != NFDIR) {
		printk(KERN_ERR "nfs4_get_root:"
		       " getroot encountered non-directory\n");
		return -ENOTDIR;
	}

	/* FIXME: It is quite valid for the server to return a referral here */
	if (fattr.valid & NFS_ATTR_FATTR_V4_REFERRAL) {
		printk(KERN_ERR "nfs4_get_root:"
		       " getroot obtained referral\n");
		return -EREMOTE;
	}

next_component:
	dprintk("Next: %s\n", path);

	/* extract the next bit of the path */
	if (!*path)
		goto path_walk_complete;

	name.name = path;
	while (*path && *path != '/')
		path++;
	name.len = path - (const char *) name.name;

eat_dot_dir:
	while (*path == '/')
		path++;

	if (path[0] == '.' && (path[1] == '/' || !path[1])) {
		path += 2;
		goto eat_dot_dir;
	}

	/* FIXME: Why shouldn't the user be able to use ".." in the path? */
	if (path[0] == '.' && path[1] == '.' && (path[2] == '/' || !path[2])
	    ) {
		printk(KERN_ERR "nfs4_get_root:"
		       " Mount path contains reference to \"..\"\n");
		return -EINVAL;
	}

	/* lookup the next FH in the sequence */
	memcpy(&lastfh, mntfh, sizeof(lastfh));

	dprintk("LookupFH: %*.*s [%s]\n", name.len, name.len, name.name, path);

	ret = server->nfs_client->rpc_ops->lookupfh(server, &lastfh, &name,
						    mntfh, &fattr);
	if (ret < 0) {
		dprintk("nfs4_get_root: getroot error = %d\n", -ret);
		return ret;
	}

	if (fattr.type != NFDIR) {
		printk(KERN_ERR "nfs4_get_root:"
		       " lookupfh encountered non-directory\n");
		return -ENOTDIR;
	}

	/* FIXME: Referrals are quite valid here too */
	if (fattr.valid & NFS_ATTR_FATTR_V4_REFERRAL) {
		printk(KERN_ERR "nfs4_get_root:"
		       " lookupfh obtained referral\n");
		return -EREMOTE;
	}

	goto next_component;

path_walk_complete:
	memcpy(&server->fsid, &fattr.fsid, sizeof(server->fsid));
	dprintk("<-- nfs4_path_walk() = 0\n");
	return 0;
}