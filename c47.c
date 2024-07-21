static int esp_input(struct xfrm_state *x, struct sk_buff *skb)
{
	struct ip_esp_hdr *esph;
	struct esp_data *esp = x->data;
	struct crypto_aead *aead = esp->aead;
	struct aead_request *req;
	struct sk_buff *trailer;
	int elen = skb->len - sizeof(*esph) - crypto_aead_ivsize(aead);
	int nfrags;
	void *tmp;
	u8 *iv;
	struct scatterlist *sg;
	struct scatterlist *asg;
	int err = -EINVAL;

	if (!pskb_may_pull(skb, sizeof(*esph)))
		goto out;

	if (elen <= 0)
		goto out;

	if ((err = skb_cow_data(skb, 0, &trailer)) < 0)
		goto out;
	nfrags = err;

	err = -ENOMEM;
	tmp = esp_alloc_tmp(aead, nfrags + 1);
	if (!tmp)
		goto out;

	ESP_SKB_CB(skb)->tmp = tmp;
	iv = esp_tmp_iv(aead, tmp);
	req = esp_tmp_req(aead, iv);
	asg = esp_req_sg(aead, req);
	sg = asg + 1;

	skb->ip_summed = CHECKSUM_NONE;

	esph = (struct ip_esp_hdr *)skb->data;

	/* Get ivec. This can be wrong, check against another impls. */
	iv = esph->enc_data;

	sg_init_table(sg, nfrags);
	skb_to_sgvec(skb, sg, sizeof(*esph) + crypto_aead_ivsize(aead), elen);
	sg_init_one(asg, esph, sizeof(*esph));

	aead_request_set_callback(req, 0, esp_input_done, skb);
	aead_request_set_crypt(req, sg, sg, elen, iv);
	aead_request_set_assoc(req, asg, sizeof(*esph));

	err = crypto_aead_decrypt(req);
	if (err == -EINPROGRESS)
		goto out;

	err = esp_input_done2(skb, err);

out:
	return err;
}