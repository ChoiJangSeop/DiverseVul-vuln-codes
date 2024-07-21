static int skcipher_recvmsg_sync(struct socket *sock, struct msghdr *msg,
				 int flags)
{
	struct sock *sk = sock->sk;
	struct alg_sock *ask = alg_sk(sk);
	struct skcipher_ctx *ctx = ask->private;
	unsigned bs = crypto_skcipher_blocksize(crypto_skcipher_reqtfm(
		&ctx->req));
	struct skcipher_sg_list *sgl;
	struct scatterlist *sg;
	int err = -EAGAIN;
	int used;
	long copied = 0;

	lock_sock(sk);
	while (msg_data_left(msg)) {
		sgl = list_first_entry(&ctx->tsgl,
				       struct skcipher_sg_list, list);
		sg = sgl->sg;

		while (!sg->length)
			sg++;

		if (!ctx->used) {
			err = skcipher_wait_for_data(sk, flags);
			if (err)
				goto unlock;
		}

		used = min_t(unsigned long, ctx->used, msg_data_left(msg));

		used = af_alg_make_sg(&ctx->rsgl, &msg->msg_iter, used);
		err = used;
		if (err < 0)
			goto unlock;

		if (ctx->more || used < ctx->used)
			used -= used % bs;

		err = -EINVAL;
		if (!used)
			goto free;

		skcipher_request_set_crypt(&ctx->req, sg, ctx->rsgl.sg, used,
					   ctx->iv);

		err = af_alg_wait_for_completion(
				ctx->enc ?
					crypto_skcipher_encrypt(&ctx->req) :
					crypto_skcipher_decrypt(&ctx->req),
				&ctx->completion);

free:
		af_alg_free_sg(&ctx->rsgl);

		if (err)
			goto unlock;

		copied += used;
		skcipher_pull_sgl(sk, used, 1);
		iov_iter_advance(&msg->msg_iter, used);
	}

	err = 0;

unlock:
	skcipher_wmem_wakeup(sk);
	release_sock(sk);

	return copied ?: err;
}