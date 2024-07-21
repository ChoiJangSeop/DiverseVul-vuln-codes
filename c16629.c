static void x509v3_cache_extensions(X509 *x)
{
	BASIC_CONSTRAINTS *bs;
	PROXY_CERT_INFO_EXTENSION *pci;
	ASN1_BIT_STRING *usage;
	ASN1_BIT_STRING *ns;
	EXTENDED_KEY_USAGE *extusage;
	X509_EXTENSION *ex;
	
	int i;
	if(x->ex_flags & EXFLAG_SET) return;
#ifndef OPENSSL_NO_SHA
	X509_digest(x, EVP_sha1(), x->sha1_hash, NULL);
#endif
	/* Does subject name match issuer ? */
	if(!X509_NAME_cmp(X509_get_subject_name(x), X509_get_issuer_name(x)))
			 x->ex_flags |= EXFLAG_SI;
	/* V1 should mean no extensions ... */
	if(!X509_get_version(x)) x->ex_flags |= EXFLAG_V1;
	/* Handle basic constraints */
	if((bs=X509_get_ext_d2i(x, NID_basic_constraints, NULL, NULL))) {
		if(bs->ca) x->ex_flags |= EXFLAG_CA;
		if(bs->pathlen) {
			if((bs->pathlen->type == V_ASN1_NEG_INTEGER)
						|| !bs->ca) {
				x->ex_flags |= EXFLAG_INVALID;
				x->ex_pathlen = 0;
			} else x->ex_pathlen = ASN1_INTEGER_get(bs->pathlen);
		} else x->ex_pathlen = -1;
		BASIC_CONSTRAINTS_free(bs);
		x->ex_flags |= EXFLAG_BCONS;
	}
	/* Handle proxy certificates */
	if((pci=X509_get_ext_d2i(x, NID_proxyCertInfo, NULL, NULL))) {
		if (x->ex_flags & EXFLAG_CA
		    || X509_get_ext_by_NID(x, NID_subject_alt_name, 0) >= 0
		    || X509_get_ext_by_NID(x, NID_issuer_alt_name, 0) >= 0) {
			x->ex_flags |= EXFLAG_INVALID;
		}
		if (pci->pcPathLengthConstraint) {
			x->ex_pcpathlen =
				ASN1_INTEGER_get(pci->pcPathLengthConstraint);
		} else x->ex_pcpathlen = -1;
		PROXY_CERT_INFO_EXTENSION_free(pci);
		x->ex_flags |= EXFLAG_PROXY;
	}
	/* Handle key usage */
	if((usage=X509_get_ext_d2i(x, NID_key_usage, NULL, NULL))) {
		if(usage->length > 0) {
			x->ex_kusage = usage->data[0];
			if(usage->length > 1) 
				x->ex_kusage |= usage->data[1] << 8;
		} else x->ex_kusage = 0;
		x->ex_flags |= EXFLAG_KUSAGE;
		ASN1_BIT_STRING_free(usage);
	}
	x->ex_xkusage = 0;
	if((extusage=X509_get_ext_d2i(x, NID_ext_key_usage, NULL, NULL))) {
		x->ex_flags |= EXFLAG_XKUSAGE;
		for(i = 0; i < sk_ASN1_OBJECT_num(extusage); i++) {
			switch(OBJ_obj2nid(sk_ASN1_OBJECT_value(extusage,i))) {
				case NID_server_auth:
				x->ex_xkusage |= XKU_SSL_SERVER;
				break;

				case NID_client_auth:
				x->ex_xkusage |= XKU_SSL_CLIENT;
				break;

				case NID_email_protect:
				x->ex_xkusage |= XKU_SMIME;
				break;

				case NID_code_sign:
				x->ex_xkusage |= XKU_CODE_SIGN;
				break;

				case NID_ms_sgc:
				case NID_ns_sgc:
				x->ex_xkusage |= XKU_SGC;
				break;

				case NID_OCSP_sign:
				x->ex_xkusage |= XKU_OCSP_SIGN;
				break;

				case NID_time_stamp:
				x->ex_xkusage |= XKU_TIMESTAMP;
				break;

				case NID_dvcs:
				x->ex_xkusage |= XKU_DVCS;
				break;
			}
		}
		sk_ASN1_OBJECT_pop_free(extusage, ASN1_OBJECT_free);
	}

	if((ns=X509_get_ext_d2i(x, NID_netscape_cert_type, NULL, NULL))) {
		if(ns->length > 0) x->ex_nscert = ns->data[0];
		else x->ex_nscert = 0;
		x->ex_flags |= EXFLAG_NSCERT;
		ASN1_BIT_STRING_free(ns);
	}
	x->skid =X509_get_ext_d2i(x, NID_subject_key_identifier, NULL, NULL);
	x->akid =X509_get_ext_d2i(x, NID_authority_key_identifier, NULL, NULL);
	x->altname = X509_get_ext_d2i(x, NID_subject_alt_name, NULL, NULL);
	x->nc = X509_get_ext_d2i(x, NID_name_constraints, &i, NULL);
	if (!x->nc && (i != -1))
		x->ex_flags |= EXFLAG_INVALID;
	setup_crldp(x);

#ifndef OPENSSL_NO_RFC3779
 	x->rfc3779_addr =X509_get_ext_d2i(x, NID_sbgp_ipAddrBlock, NULL, NULL);
 	x->rfc3779_asid =X509_get_ext_d2i(x, NID_sbgp_autonomousSysNum,
 					  NULL, NULL);
#endif
	for (i = 0; i < X509_get_ext_count(x); i++)
		{
		ex = X509_get_ext(x, i);
		if (OBJ_obj2nid(X509_EXTENSION_get_object(ex))
					== NID_freshest_crl)
			x->ex_flags |= EXFLAG_FRESHEST;
		if (!X509_EXTENSION_get_critical(ex))
			continue;
		if (!X509_supported_extension(ex))
			{
			x->ex_flags |= EXFLAG_CRITICAL;
			break;
			}
		}
	x->ex_flags |= EXFLAG_SET;
}