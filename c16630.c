int X509_check_trust(X509 *x, int id, int flags)
{
	X509_TRUST *pt;
	int idx;
	if(id == -1) return 1;
	idx = X509_TRUST_get_by_id(id);
	if(idx == -1) return default_trust(id, x, flags);
	pt = X509_TRUST_get0(idx);
	return pt->check_trust(pt, x, flags);
}