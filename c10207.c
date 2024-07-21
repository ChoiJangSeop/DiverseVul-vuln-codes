_lasso_check_assertion_issuer(LassoSaml2Assertion *assertion, const gchar *provider_id)
{
	if (! LASSO_SAML2_ASSERTION(assertion) || ! provider_id)
		return FALSE;

	if (! assertion->Issuer || ! assertion->Issuer->content)
		return FALSE;

	return lasso_strisequal(assertion->Issuer->content,provider_id);
}