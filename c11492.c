
static void io_clean_op(struct io_kiocb *req)
{
	if (req->flags & REQ_F_BUFFER_SELECTED) {
		spin_lock(&req->ctx->completion_lock);
		io_put_kbuf_comp(req);
		spin_unlock(&req->ctx->completion_lock);
	}

	if (req->flags & REQ_F_NEED_CLEANUP) {
		switch (req->opcode) {
		case IORING_OP_READV:
		case IORING_OP_READ_FIXED:
		case IORING_OP_READ:
		case IORING_OP_WRITEV:
		case IORING_OP_WRITE_FIXED:
		case IORING_OP_WRITE: {
			struct io_async_rw *io = req->async_data;

			kfree(io->free_iovec);
			break;
			}
		case IORING_OP_RECVMSG:
		case IORING_OP_SENDMSG: {
			struct io_async_msghdr *io = req->async_data;

			kfree(io->free_iov);
			break;
			}
		case IORING_OP_OPENAT:
		case IORING_OP_OPENAT2:
			if (req->open.filename)
				putname(req->open.filename);
			break;
		case IORING_OP_RENAMEAT:
			putname(req->rename.oldpath);
			putname(req->rename.newpath);
			break;
		case IORING_OP_UNLINKAT:
			putname(req->unlink.filename);
			break;
		case IORING_OP_MKDIRAT:
			putname(req->mkdir.filename);
			break;
		case IORING_OP_SYMLINKAT:
			putname(req->symlink.oldpath);
			putname(req->symlink.newpath);
			break;
		case IORING_OP_LINKAT:
			putname(req->hardlink.oldpath);
			putname(req->hardlink.newpath);
			break;
		case IORING_OP_STATX:
			if (req->statx.filename)
				putname(req->statx.filename);
			break;
		case IORING_OP_SETXATTR:
		case IORING_OP_FSETXATTR:
		case IORING_OP_GETXATTR:
		case IORING_OP_FGETXATTR:
			__io_xattr_finish(req);
			break;
		}
	}
	if ((req->flags & REQ_F_POLLED) && req->apoll) {
		kfree(req->apoll->double_poll);
		kfree(req->apoll);
		req->apoll = NULL;
	}
	if (req->flags & REQ_F_CREDS)
		put_cred(req->creds);
	if (req->flags & REQ_F_ASYNC_DATA) {
		kfree(req->async_data);
		req->async_data = NULL;
	}
	req->flags &= ~IO_REQ_CLEAN_FLAGS;