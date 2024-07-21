PJ_DEF(void) pjsip_auth_create_digestSHA256(pj_str_t *result,
					    const pj_str_t *nonce,
					    const pj_str_t *nc,
					    const pj_str_t *cnonce,
					    const pj_str_t *qop,
					    const pj_str_t *uri,
					    const pj_str_t *realm,
					    const pjsip_cred_info *cred_info,
					    const pj_str_t *method)
{
#if PJSIP_AUTH_HAS_DIGEST_SHA256

    char ha1[PJSIP_SHA256STRLEN];
    char ha2[PJSIP_SHA256STRLEN];
    unsigned char digest[32];
    SHA256_CTX pms;

    pj_assert(result->slen >= PJSIP_SHA256STRLEN);

    AUTH_TRACE_((THIS_FILE, "Begin creating digest"));

    if ((cred_info->data_type & PASSWD_MASK) == PJSIP_CRED_DATA_PLAIN_PASSWD)
    {
	/***
	 *** ha1 = SHA256(username ":" realm ":" password)
	 ***/
	SHA256_Init(&pms);
	SHA256_Update( &pms, cred_info->username.ptr,
		       cred_info->username.slen);
	SHA256_Update( &pms, ":", 1);
	SHA256_Update( &pms, realm->ptr, realm->slen);
	SHA256_Update( &pms, ":", 1);
	SHA256_Update( &pms, cred_info->data.ptr, cred_info->data.slen);
	SHA256_Final(digest, &pms);

	digestNtoStr(digest, 32, ha1);

    } else if ((cred_info->data_type & PASSWD_MASK) == PJSIP_CRED_DATA_DIGEST)
    {
	pj_assert(cred_info->data.slen == 32);
	pj_memcpy( ha1, cred_info->data.ptr, cred_info->data.slen );
    } else {
	pj_assert(!"Invalid data_type");
    }

    AUTH_TRACE_((THIS_FILE, " ha1=%.64s", ha1));

    /***
     *** ha2 = SHA256(method ":" req_uri)
     ***/
    SHA256_Init(&pms);
    SHA256_Update( &pms, method->ptr, method->slen);
    SHA256_Update( &pms, ":", 1);
    SHA256_Update( &pms, uri->ptr, uri->slen);
    SHA256_Final( digest, &pms);
    digestNtoStr(digest, 32, ha2);

    AUTH_TRACE_((THIS_FILE, " ha2=%.64s", ha2));

    /***
     *** When qop is not used:
     ***   response = SHA256(ha1 ":" nonce ":" ha2)
     ***
     *** When qop=auth is used:
     ***   response = SHA256(ha1 ":" nonce ":" nc ":" cnonce ":" qop ":" ha2)
     ***/
    SHA256_Init(&pms);
    SHA256_Update( &pms, ha1, PJSIP_SHA256STRLEN);
    SHA256_Update( &pms, ":", 1);
    SHA256_Update( &pms, nonce->ptr, nonce->slen);
    if (qop && qop->slen != 0) {
	SHA256_Update( &pms, ":", 1);
	SHA256_Update( &pms, nc->ptr, nc->slen);
	SHA256_Update( &pms, ":", 1);
	SHA256_Update( &pms, cnonce->ptr, cnonce->slen);
	SHA256_Update( &pms, ":", 1);
	SHA256_Update( &pms, qop->ptr, qop->slen);
    }
    SHA256_Update( &pms, ":", 1);
    SHA256_Update( &pms, ha2, PJSIP_SHA256STRLEN);

    /* This is the final response digest. */
    SHA256_Final(digest, &pms);

    /* Convert digest to string and store in chal->response. */
    result->slen = PJSIP_SHA256STRLEN;
    digestNtoStr(digest, 32, result->ptr);

    AUTH_TRACE_((THIS_FILE, " digest=%.64s", result->ptr));
    AUTH_TRACE_((THIS_FILE, "Digest created"));

#else
    PJ_UNUSED_ARG(result);
    PJ_UNUSED_ARG(nonce);
    PJ_UNUSED_ARG(nc);
    PJ_UNUSED_ARG(cnonce);
    PJ_UNUSED_ARG(qop);
    PJ_UNUSED_ARG(uri);
    PJ_UNUSED_ARG(realm);
    PJ_UNUSED_ARG(cred_info);
    PJ_UNUSED_ARG(method);
#endif
}