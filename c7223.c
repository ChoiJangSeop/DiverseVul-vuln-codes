dlz_allowzonexfr(void *dbdata, const char *name, const char *client) {
	UNUSED(client);

	/* Just say yes for all our zones */
	return (dlz_findzonedb(dbdata, name, NULL, NULL));
}