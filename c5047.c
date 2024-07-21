static int ahash_def_finup(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	int err;

	err = ahash_save_req(req, ahash_def_finup_done1);
	if (err)
		return err;

	err = tfm->update(req);
	return ahash_def_finup_finish1(req, err);
}