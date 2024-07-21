static void __fanout_unlink(struct sock *sk, struct packet_sock *po)
{
	struct packet_fanout *f = po->fanout;
	int i;

	spin_lock(&f->lock);
	for (i = 0; i < f->num_members; i++) {
		if (f->arr[i] == sk)
			break;
	}
	BUG_ON(i >= f->num_members);
	f->arr[i] = f->arr[f->num_members - 1];
	f->num_members--;
	spin_unlock(&f->lock);
}