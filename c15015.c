static pj_status_t respond_digest( pj_pool_t *pool,
				   pjsip_digest_credential *cred,
				   const pjsip_digest_challenge *chal,
				   const pj_str_t *uri,
				   const pjsip_cred_info *cred_info,
				   const pj_str_t *cnonce,
				   pj_uint32_t nc,
				   const pj_str_t *method)
{
    const pj_str_t pjsip_AKAv1_MD5_STR = { "AKAv1-MD5", 9 };
    pj_bool_t algo_sha256 = PJ_FALSE;

    /* Check if algo is sha256 */
#if PJSIP_AUTH_HAS_DIGEST_SHA256
    algo_sha256 = (pj_stricmp(&chal->algorithm, &pjsip_SHA256_STR)==0);
#endif

    /* Check algorithm is supported. We support MD5, AKAv1-MD5, and SHA256. */
    if (chal->algorithm.slen==0 ||
        (algo_sha256 ||
	 pj_stricmp(&chal->algorithm, &pjsip_MD5_STR)==0 ||
         pj_stricmp(&chal->algorithm, &pjsip_AKAv1_MD5_STR)==0))
    {
	PJ_LOG(4,(THIS_FILE, "Digest algorithm is \"%.*s\"",
		  chal->algorithm.slen, chal->algorithm.ptr));
    }
    else {
	PJ_LOG(4,(THIS_FILE, "Unsupported digest algorithm \"%.*s\"",
		  chal->algorithm.slen, chal->algorithm.ptr));
	return PJSIP_EINVALIDALGORITHM;
    }

    /* Build digest credential from arguments. */
    pj_strdup(pool, &cred->username, &cred_info->username);
    pj_strdup(pool, &cred->realm, &chal->realm);
    pj_strdup(pool, &cred->nonce, &chal->nonce);
    pj_strdup(pool, &cred->uri, uri);
    pj_strdup(pool, &cred->algorithm, &chal->algorithm);
    pj_strdup(pool, &cred->opaque, &chal->opaque);

    /* Allocate memory. */
    cred->response.slen = algo_sha256? PJSIP_SHA256STRLEN : PJSIP_MD5STRLEN;
    cred->response.ptr = (char*) pj_pool_alloc(pool, cred->response.slen);

    if (chal->qop.slen == 0) {
	/* Server doesn't require quality of protection. */

	if ((cred_info->data_type & EXT_MASK) == PJSIP_CRED_DATA_EXT_AKA) {
	    /* Call application callback to create the response digest */
	    return (*cred_info->ext.aka.cb)(pool, chal, cred_info,
					    method, cred);
	}
	else {
	    /* Convert digest to string and store in chal->response. */
	    if (algo_sha256) {
		pjsip_auth_create_digestSHA256(
					  &cred->response, &cred->nonce, NULL,
					  NULL,  NULL, uri, &chal->realm,
					  cred_info, method);
	    } else {
		pjsip_auth_create_digest( &cred->response, &cred->nonce, NULL,
					  NULL,  NULL, uri, &chal->realm,
					  cred_info, method);
	    }
	}

    } else if (has_auth_qop(pool, &chal->qop)) {
	/* Server requires quality of protection.
	 * We respond with selecting "qop=auth" protection.
	 */
	cred->qop = pjsip_AUTH_STR;
	cred->nc.ptr = (char*) pj_pool_alloc(pool, 16);
	cred->nc.slen = pj_ansi_snprintf(cred->nc.ptr, 16, "%08u", nc);

	if (cnonce && cnonce->slen) {
	    pj_strdup(pool, &cred->cnonce, cnonce);
	} else {
	    pj_str_t dummy_cnonce = { "b39971", 6};
	    pj_strdup(pool, &cred->cnonce, &dummy_cnonce);
	}

	if ((cred_info->data_type & EXT_MASK) == PJSIP_CRED_DATA_EXT_AKA) {
	    /* Call application callback to create the response digest */
	    return (*cred_info->ext.aka.cb)(pool, chal, cred_info,
					    method, cred);
	}
	else {
	    /* Convert digest to string and store in chal->response. */
	    if (algo_sha256) {
		pjsip_auth_create_digestSHA256(
					  &cred->response, &cred->nonce,
					  &cred->nc, &cred->cnonce,
					  &pjsip_AUTH_STR, uri,
					  &chal->realm, cred_info,
					  method);
	    } else {
		pjsip_auth_create_digest( &cred->response, &cred->nonce,
					  &cred->nc, &cred->cnonce,
					  &pjsip_AUTH_STR, uri,
					  &chal->realm, cred_info,
					  method);
	    }
	}

    } else {
	/* Server requires quality protection that we don't support. */
	PJ_LOG(4,(THIS_FILE, "Unsupported qop offer %.*s",
		  chal->qop.slen, chal->qop.ptr));
	return PJSIP_EINVALIDQOP;
    }

    return PJ_SUCCESS;
}