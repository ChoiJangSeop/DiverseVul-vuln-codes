static void ahash_op_unaligned_done(struct crypto_async_request *req, int err)
{
	struct ahash_request *areq = req->data;

	/*
	 * Restore the original request, see ahash_op_unaligned() for what
	 * goes where.
	 *
	 * The "struct ahash_request *req" here is in fact the "req.base"
	 * from the ADJUSTED request from ahash_op_unaligned(), thus as it
	 * is a pointer to self, it is also the ADJUSTED "req" .
	 */

	/* First copy req->result into req->priv.result */
	ahash_op_unaligned_finish(areq, err);

	/* Complete the ORIGINAL request. */
	areq->base.complete(&areq->base, err);
}