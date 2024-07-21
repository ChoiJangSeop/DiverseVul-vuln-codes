static int ahash_def_finup_finish1(struct ahash_request *req, int err)
{
	if (err)
		goto out;

	req->base.complete = ahash_def_finup_done2;
	req->base.flags &= ~CRYPTO_TFM_REQ_MAY_SLEEP;
	err = crypto_ahash_reqtfm(req)->final(req);

out:
	ahash_def_finup_finish2(req, err);
	return err;
}