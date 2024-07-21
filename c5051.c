static void ahash_def_finup_done1(struct crypto_async_request *req, int err)
{
	struct ahash_request *areq = req->data;

	err = ahash_def_finup_finish1(areq, err);

	areq->base.complete(&areq->base, err);
}