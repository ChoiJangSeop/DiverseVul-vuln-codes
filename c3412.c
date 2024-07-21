list_restrict4(
	restrict_u *		res,
	struct info_restrict **	ppir
	)
{
	struct info_restrict *	pir;

	if (res->link != NULL)
		list_restrict4(res->link, ppir);

	pir = *ppir;
	pir->addr = htonl(res->u.v4.addr);
	if (client_v6_capable) 
		pir->v6_flag = 0;
	pir->mask = htonl(res->u.v4.mask);
	pir->count = htonl(res->count);
	pir->flags = htons(res->flags);
	pir->mflags = htons(res->mflags);
	*ppir = (struct info_restrict *)more_pkt();
}