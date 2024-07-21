void dane_query_deinit(dane_query_t q)
{
	ub_resolve_free(q->result);
	free(q);
}