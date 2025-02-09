static int llcp_sock_create(struct net *net, struct socket *sock,
			    const struct nfc_protocol *nfc_proto, int kern)
{
	struct sock *sk;

	pr_debug("%p\n", sock);

	if (sock->type != SOCK_STREAM &&
	    sock->type != SOCK_DGRAM &&
	    sock->type != SOCK_RAW)
		return -ESOCKTNOSUPPORT;

	if (sock->type == SOCK_RAW)
		sock->ops = &llcp_rawsock_ops;
	else
		sock->ops = &llcp_sock_ops;

	sk = nfc_llcp_sock_alloc(sock, sock->type, GFP_ATOMIC, kern);
	if (sk == NULL)
		return -ENOMEM;

	return 0;
}