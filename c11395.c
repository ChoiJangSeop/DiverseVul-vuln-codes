void x25_kill_by_neigh(struct x25_neigh *nb)
{
	struct sock *s;

	write_lock_bh(&x25_list_lock);

	sk_for_each(s, &x25_list)
		if (x25_sk(s)->neighbour == nb)
			x25_disconnect(s, ENETUNREACH, 0, 0);

	write_unlock_bh(&x25_list_lock);

	/* Remove any related forwards */
	x25_clear_forward_by_dev(nb->dev);
}