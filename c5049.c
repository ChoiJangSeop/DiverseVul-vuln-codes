static void ahash_restore_req(struct ahash_request *req)
{
	struct ahash_request_priv *priv = req->priv;

	/* Restore the original crypto request. */
	req->result = priv->result;
	req->base.complete = priv->complete;
	req->base.data = priv->data;
	req->priv = NULL;

	/* Free the req->priv.priv from the ADJUSTED request. */
	kzfree(priv);
}