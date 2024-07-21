static void maybe_unmark_and_push(struct sock *x)
{
	struct unix_sock *u = unix_sk(x);

	if (u->gc_tree != GC_ORPHAN)
		return;
	sock_hold(x);
	u->gc_tree = gc_current;
	gc_current = x;
}