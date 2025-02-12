static int crypto_report_comp(struct sk_buff *skb, struct crypto_alg *alg)
{
	struct crypto_report_comp rcomp;

	snprintf(rcomp.type, CRYPTO_MAX_ALG_NAME, "%s", "compression");

	if (nla_put(skb, CRYPTOCFGA_REPORT_COMPRESS,
		    sizeof(struct crypto_report_comp), &rcomp))
		goto nla_put_failure;
	return 0;

nla_put_failure:
	return -EMSGSIZE;
}