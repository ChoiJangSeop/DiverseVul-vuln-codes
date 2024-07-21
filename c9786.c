static int io_uring_get_fd(struct io_ring_ctx *ctx)
{
	struct file *file;
	int ret;

#if defined(CONFIG_UNIX)
	ret = sock_create_kern(&init_net, PF_UNIX, SOCK_RAW, IPPROTO_IP,
				&ctx->ring_sock);
	if (ret)
		return ret;
#endif

	ret = get_unused_fd_flags(O_RDWR | O_CLOEXEC);
	if (ret < 0)
		goto err;

	file = anon_inode_getfile("[io_uring]", &io_uring_fops, ctx,
					O_RDWR | O_CLOEXEC);
	if (IS_ERR(file)) {
		put_unused_fd(ret);
		ret = PTR_ERR(file);
		goto err;
	}

#if defined(CONFIG_UNIX)
	ctx->ring_sock->file = file;
#endif
	fd_install(ret, file);
	return ret;
err:
#if defined(CONFIG_UNIX)
	sock_release(ctx->ring_sock);
	ctx->ring_sock = NULL;
#endif
	return ret;
}