int X509_verify(X509 *a, EVP_PKEY *r)
	{
	return(ASN1_item_verify(ASN1_ITEM_rptr(X509_CINF),a->sig_alg,
		a->signature,a->cert_info,r));
	}