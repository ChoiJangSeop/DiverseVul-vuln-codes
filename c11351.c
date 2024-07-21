static int llc_ui_autobind(struct socket *sock, struct sockaddr_llc *addr)
{
	struct sock *sk = sock->sk;
	struct llc_sock *llc = llc_sk(sk);
	struct llc_sap *sap;
	int rc = -EINVAL;

	if (!sock_flag(sk, SOCK_ZAPPED))
		goto out;
	if (!addr->sllc_arphrd)
		addr->sllc_arphrd = ARPHRD_ETHER;
	if (addr->sllc_arphrd != ARPHRD_ETHER)
		goto out;
	rc = -ENODEV;
	if (sk->sk_bound_dev_if) {
		llc->dev = dev_get_by_index(&init_net, sk->sk_bound_dev_if);
		if (llc->dev && addr->sllc_arphrd != llc->dev->type) {
			dev_put(llc->dev);
			llc->dev = NULL;
		}
	} else
		llc->dev = dev_getfirstbyhwtype(&init_net, addr->sllc_arphrd);
	if (!llc->dev)
		goto out;
	netdev_tracker_alloc(llc->dev, &llc->dev_tracker, GFP_KERNEL);
	rc = -EUSERS;
	llc->laddr.lsap = llc_ui_autoport();
	if (!llc->laddr.lsap)
		goto out;
	rc = -EBUSY; /* some other network layer is using the sap */
	sap = llc_sap_open(llc->laddr.lsap, NULL);
	if (!sap)
		goto out;
	memcpy(llc->laddr.mac, llc->dev->dev_addr, IFHWADDRLEN);
	memcpy(&llc->addr, addr, sizeof(llc->addr));
	/* assign new connection to its SAP */
	llc_sap_add_socket(sap, sk);
	sock_reset_flag(sk, SOCK_ZAPPED);
	rc = 0;
out:
	return rc;
}