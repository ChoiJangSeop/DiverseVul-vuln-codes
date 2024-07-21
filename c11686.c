static void rose_remove_neigh(struct rose_neigh *rose_neigh)
{
	struct rose_neigh *s;

	rose_stop_ftimer(rose_neigh);
	rose_stop_t0timer(rose_neigh);

	skb_queue_purge(&rose_neigh->queue);

	if ((s = rose_neigh_list) == rose_neigh) {
		rose_neigh_list = rose_neigh->next;
		if (rose_neigh->ax25)
			ax25_cb_put(rose_neigh->ax25);
		kfree(rose_neigh->digipeat);
		kfree(rose_neigh);
		return;
	}

	while (s != NULL && s->next != NULL) {
		if (s->next == rose_neigh) {
			s->next = rose_neigh->next;
			if (rose_neigh->ax25)
				ax25_cb_put(rose_neigh->ax25);
			kfree(rose_neigh->digipeat);
			kfree(rose_neigh);
			return;
		}

		s = s->next;
	}
}