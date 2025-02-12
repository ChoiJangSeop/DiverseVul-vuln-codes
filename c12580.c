static int crypto_pcomp_report(struct sk_buff *skb, struct crypto_alg *alg)
{
	struct crypto_report_comp rpcomp;

	snprintf(rpcomp.type, CRYPTO_MAX_ALG_NAME, "%s", "pcomp");

	if (nla_put(skb, CRYPTOCFGA_REPORT_COMPRESS,
		    sizeof(struct crypto_report_comp), &rpcomp))
		goto nla_put_failure;
	return 0;

nla_put_failure:
	return -EMSGSIZE;
}