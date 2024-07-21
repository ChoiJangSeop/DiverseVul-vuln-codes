static int ahash_save_req(struct ahash_request *req, crypto_completion_t cplt)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	unsigned long alignmask = crypto_ahash_alignmask(tfm);
	unsigned int ds = crypto_ahash_digestsize(tfm);
	struct ahash_request_priv *priv;

	priv = kmalloc(sizeof(*priv) + ahash_align_buffer_size(ds, alignmask),
		       (req->base.flags & CRYPTO_TFM_REQ_MAY_SLEEP) ?
		       GFP_KERNEL : GFP_ATOMIC);
	if (!priv)
		return -ENOMEM;

	/*
	 * WARNING: Voodoo programming below!
	 *
	 * The code below is obscure and hard to understand, thus explanation
	 * is necessary. See include/crypto/hash.h and include/linux/crypto.h
	 * to understand the layout of structures used here!
	 *
	 * The code here will replace portions of the ORIGINAL request with
	 * pointers to new code and buffers so the hashing operation can store
	 * the result in aligned buffer. We will call the modified request
	 * an ADJUSTED request.
	 *
	 * The newly mangled request will look as such:
	 *
	 * req {
	 *   .result        = ADJUSTED[new aligned buffer]
	 *   .base.complete = ADJUSTED[pointer to completion function]
	 *   .base.data     = ADJUSTED[*req (pointer to self)]
	 *   .priv          = ADJUSTED[new priv] {
	 *           .result   = ORIGINAL(result)
	 *           .complete = ORIGINAL(base.complete)
	 *           .data     = ORIGINAL(base.data)
	 *   }
	 */

	priv->result = req->result;
	priv->complete = req->base.complete;
	priv->data = req->base.data;
	/*
	 * WARNING: We do not backup req->priv here! The req->priv
	 *          is for internal use of the Crypto API and the
	 *          user must _NOT_ _EVER_ depend on it's content!
	 */

	req->result = PTR_ALIGN((u8 *)priv->ubuf, alignmask + 1);
	req->base.complete = cplt;
	req->base.data = req;
	req->priv = priv;

	return 0;
}