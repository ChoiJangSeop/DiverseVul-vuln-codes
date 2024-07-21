list_restrict6(
	restrict_u *		res,
	struct info_restrict **	ppir
	)
{
	struct info_restrict *	pir;

	if (res->link != NULL)
		list_restrict6(res->link, ppir);

	pir = *ppir;
	pir->addr6 = res->u.v6.addr; 
	pir->mask6 = res->u.v6.mask;
	pir->v6_flag = 1;
	pir->count = htonl(res->count);
	pir->flags = htons(res->flags);
	pir->mflags = htons(res->mflags);
	*ppir = (struct info_restrict *)more_pkt();
}