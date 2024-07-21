int selinux_netlbl_inode_permission(struct inode *inode, int mask)
{
	int rc;
	struct sock *sk;
	struct socket *sock;
	struct sk_security_struct *sksec;

	if (!S_ISSOCK(inode->i_mode) ||
	    ((mask & (MAY_WRITE | MAY_APPEND)) == 0))
		return 0;

	sock = SOCKET_I(inode);
	sk = sock->sk;
	sksec = sk->sk_security;
	if (sksec->nlbl_state != NLBL_REQUIRE)
		return 0;

	local_bh_disable();
	bh_lock_sock_nested(sk);
	if (likely(sksec->nlbl_state == NLBL_REQUIRE))
		rc = selinux_netlbl_sock_setsid(sk);
	else
		rc = 0;
	bh_unlock_sock(sk);
	local_bh_enable();

	return rc;
}