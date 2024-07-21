PJ_DEF(void) pjsip_auth_create_digest( pj_str_t *result,
				       const pj_str_t *nonce,
				       const pj_str_t *nc,
				       const pj_str_t *cnonce,
				       const pj_str_t *qop,
				       const pj_str_t *uri,
				       const pj_str_t *realm,
				       const pjsip_cred_info *cred_info,
				       const pj_str_t *method)
{
    char ha1[PJSIP_MD5STRLEN];
    char ha2[PJSIP_MD5STRLEN];
    unsigned char digest[16];
    pj_md5_context pms;

    pj_assert(result->slen >= PJSIP_MD5STRLEN);

    AUTH_TRACE_((THIS_FILE, "Begin creating digest"));

    if ((cred_info->data_type & PASSWD_MASK) == PJSIP_CRED_DATA_PLAIN_PASSWD) {
	/***
	 *** ha1 = MD5(username ":" realm ":" password)
	 ***/
	pj_md5_init(&pms);
	MD5_APPEND( &pms, cred_info->username.ptr, cred_info->username.slen);
	MD5_APPEND( &pms, ":", 1);
	MD5_APPEND( &pms, realm->ptr, realm->slen);
	MD5_APPEND( &pms, ":", 1);
	MD5_APPEND( &pms, cred_info->data.ptr, cred_info->data.slen);
	pj_md5_final(&pms, digest);

	digestNtoStr(digest, 16, ha1);

    } else if ((cred_info->data_type & PASSWD_MASK) == PJSIP_CRED_DATA_DIGEST) {
	pj_assert(cred_info->data.slen == 32);
	pj_memcpy( ha1, cred_info->data.ptr, cred_info->data.slen );
    } else {
	pj_assert(!"Invalid data_type");
    }

    AUTH_TRACE_((THIS_FILE, "  ha1=%.32s", ha1));

    /***
     *** ha2 = MD5(method ":" req_uri)
     ***/
    pj_md5_init(&pms);
    MD5_APPEND( &pms, method->ptr, method->slen);
    MD5_APPEND( &pms, ":", 1);
    MD5_APPEND( &pms, uri->ptr, uri->slen);
    pj_md5_final(&pms, digest);
    digestNtoStr(digest, 16, ha2);

    AUTH_TRACE_((THIS_FILE, "  ha2=%.32s", ha2));

    /***
     *** When qop is not used:
     ***    response = MD5(ha1 ":" nonce ":" ha2)
     ***
     *** When qop=auth is used:
     ***    response = MD5(ha1 ":" nonce ":" nc ":" cnonce ":" qop ":" ha2)
     ***/
    pj_md5_init(&pms);
    MD5_APPEND( &pms, ha1, PJSIP_MD5STRLEN);
    MD5_APPEND( &pms, ":", 1);
    MD5_APPEND( &pms, nonce->ptr, nonce->slen);
    if (qop && qop->slen != 0) {
	MD5_APPEND( &pms, ":", 1);
	MD5_APPEND( &pms, nc->ptr, nc->slen);
	MD5_APPEND( &pms, ":", 1);
	MD5_APPEND( &pms, cnonce->ptr, cnonce->slen);
	MD5_APPEND( &pms, ":", 1);
	MD5_APPEND( &pms, qop->ptr, qop->slen);
    }
    MD5_APPEND( &pms, ":", 1);
    MD5_APPEND( &pms, ha2, PJSIP_MD5STRLEN);

    /* This is the final response digest. */
    pj_md5_final(&pms, digest);

    /* Convert digest to string and store in chal->response. */
    result->slen = PJSIP_MD5STRLEN;
    digestNtoStr(digest, 16, result->ptr);

    AUTH_TRACE_((THIS_FILE, "  digest=%.32s", result->ptr));
    AUTH_TRACE_((THIS_FILE, "Digest created"));
}