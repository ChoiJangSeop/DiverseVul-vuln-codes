static int crypto_nivaead_report(struct sk_buff *skb, struct crypto_alg *alg)
{
	struct crypto_report_aead raead;
	struct aead_alg *aead = &alg->cra_aead;

	snprintf(raead.type, CRYPTO_MAX_ALG_NAME, "%s", "nivaead");
	snprintf(raead.geniv, CRYPTO_MAX_ALG_NAME, "%s", aead->geniv);

	raead.blocksize = alg->cra_blocksize;
	raead.maxauthsize = aead->maxauthsize;
	raead.ivsize = aead->ivsize;

	if (nla_put(skb, CRYPTOCFGA_REPORT_AEAD,
		    sizeof(struct crypto_report_aead), &raead))
		goto nla_put_failure;
	return 0;

nla_put_failure:
	return -EMSGSIZE;
}