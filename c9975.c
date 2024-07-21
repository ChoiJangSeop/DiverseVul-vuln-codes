authzPretty(
	Syntax *syntax,
	struct berval *val,
	struct berval *out,
	void *ctx)
{
	int		rc;

	Debug( LDAP_DEBUG_TRACE, ">>> authzPretty: <%s>\n",
		val->bv_val, 0, 0 );

	rc = authzPrettyNormal( val, out, ctx, 0 );

	Debug( LDAP_DEBUG_TRACE, "<<< authzPretty: <%s> (%d)\n",
		out->bv_val, rc, 0 );

	return rc;
}