static void ahash_def_finup_done2(struct crypto_async_request *req, int err)
{
	struct ahash_request *areq = req->data;

	ahash_def_finup_finish2(areq, err);

	areq->base.complete(&areq->base, err);
}