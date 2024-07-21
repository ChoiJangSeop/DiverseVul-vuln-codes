static inline __u16	inet_getid(struct inet_peer *p, int more)
{
	more++;
	inet_peer_refcheck(p);
	return atomic_add_return(more, &p->ip_id_count) - more;
}