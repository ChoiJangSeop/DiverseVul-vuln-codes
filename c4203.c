void inet6_destroy_sock(struct sock *sk)
{
	struct ipv6_pinfo *np = inet6_sk(sk);
	struct sk_buff *skb;
	struct ipv6_txoptions *opt;

	/* Release rx options */

	skb = xchg(&np->pktoptions, NULL);
	if (skb)
		kfree_skb(skb);

	skb = xchg(&np->rxpmtu, NULL);
	if (skb)
		kfree_skb(skb);

	/* Free flowlabels */
	fl6_free_socklist(sk);

	/* Free tx options */

	opt = xchg(&np->opt, NULL);
	if (opt)
		sock_kfree_s(sk, opt, opt->tot_len);
}