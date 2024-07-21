void ssl_set_cert_masks(CERT *c, const SSL_CIPHER *cipher)
	{
	CERT_PKEY *cpk;
	int rsa_enc,rsa_tmp,rsa_sign,dh_tmp,dh_rsa,dh_dsa,dsa_sign;
	int rsa_enc_export,dh_rsa_export,dh_dsa_export;
	int rsa_tmp_export,dh_tmp_export,kl;
	unsigned long mask_k,mask_a,emask_k,emask_a;
	int have_ecc_cert, ecdh_ok, ecdsa_ok, ecc_pkey_size;
#ifndef OPENSSL_NO_ECDH
	int have_ecdh_tmp;
#endif
	X509 *x = NULL;
	EVP_PKEY *ecc_pkey = NULL;
	int signature_nid = 0, pk_nid = 0, md_nid = 0;

	if (c == NULL) return;

	kl=SSL_C_EXPORT_PKEYLENGTH(cipher);

#ifndef OPENSSL_NO_RSA
	rsa_tmp=(c->rsa_tmp != NULL || c->rsa_tmp_cb != NULL);
	rsa_tmp_export=(c->rsa_tmp_cb != NULL ||
		(rsa_tmp && RSA_size(c->rsa_tmp)*8 <= kl));
#else
	rsa_tmp=rsa_tmp_export=0;
#endif
#ifndef OPENSSL_NO_DH
	dh_tmp=(c->dh_tmp != NULL || c->dh_tmp_cb != NULL);
	dh_tmp_export=(c->dh_tmp_cb != NULL ||
		(dh_tmp && DH_size(c->dh_tmp)*8 <= kl));
#else
	dh_tmp=dh_tmp_export=0;
#endif

#ifndef OPENSSL_NO_ECDH
	have_ecdh_tmp=(c->ecdh_tmp != NULL || c->ecdh_tmp_cb != NULL);
#endif
	cpk= &(c->pkeys[SSL_PKEY_RSA_ENC]);
	rsa_enc= (cpk->x509 != NULL && cpk->privatekey != NULL);
	rsa_enc_export=(rsa_enc && EVP_PKEY_size(cpk->privatekey)*8 <= kl);
	cpk= &(c->pkeys[SSL_PKEY_RSA_SIGN]);
	rsa_sign=(cpk->x509 != NULL && cpk->privatekey != NULL);
	cpk= &(c->pkeys[SSL_PKEY_DSA_SIGN]);
	dsa_sign=(cpk->x509 != NULL && cpk->privatekey != NULL);
	cpk= &(c->pkeys[SSL_PKEY_DH_RSA]);
	dh_rsa=  (cpk->x509 != NULL && cpk->privatekey != NULL);
	dh_rsa_export=(dh_rsa && EVP_PKEY_size(cpk->privatekey)*8 <= kl);
	cpk= &(c->pkeys[SSL_PKEY_DH_DSA]);
/* FIX THIS EAY EAY EAY */
	dh_dsa=  (cpk->x509 != NULL && cpk->privatekey != NULL);
	dh_dsa_export=(dh_dsa && EVP_PKEY_size(cpk->privatekey)*8 <= kl);
	cpk= &(c->pkeys[SSL_PKEY_ECC]);
	have_ecc_cert= (cpk->x509 != NULL && cpk->privatekey != NULL);
	mask_k=0;
	mask_a=0;
	emask_k=0;
	emask_a=0;

	

#ifdef CIPHER_DEBUG
	printf("rt=%d rte=%d dht=%d ecdht=%d re=%d ree=%d rs=%d ds=%d dhr=%d dhd=%d\n",
	        rsa_tmp,rsa_tmp_export,dh_tmp,have_ecdh_tmp,
		rsa_enc,rsa_enc_export,rsa_sign,dsa_sign,dh_rsa,dh_dsa);
#endif
	
	cpk = &(c->pkeys[SSL_PKEY_GOST01]);
	if (cpk->x509 != NULL && cpk->privatekey !=NULL) {
		mask_k |= SSL_kGOST;
		mask_a |= SSL_aGOST01;
	}
	cpk = &(c->pkeys[SSL_PKEY_GOST94]);
	if (cpk->x509 != NULL && cpk->privatekey !=NULL) {
		mask_k |= SSL_kGOST;
		mask_a |= SSL_aGOST94;
	}

	if (rsa_enc || (rsa_tmp && rsa_sign))
		mask_k|=SSL_kRSA;
	if (rsa_enc_export || (rsa_tmp_export && (rsa_sign || rsa_enc)))
		emask_k|=SSL_kRSA;

#if 0
	/* The match needs to be both kEDH and aRSA or aDSA, so don't worry */
	if (	(dh_tmp || dh_rsa || dh_dsa) &&
		(rsa_enc || rsa_sign || dsa_sign))
		mask_k|=SSL_kEDH;
	if ((dh_tmp_export || dh_rsa_export || dh_dsa_export) &&
		(rsa_enc || rsa_sign || dsa_sign))
		emask_k|=SSL_kEDH;
#endif

	if (dh_tmp_export)
		emask_k|=SSL_kEDH;

	if (dh_tmp)
		mask_k|=SSL_kEDH;

	if (dh_rsa) mask_k|=SSL_kDHr;
	if (dh_rsa_export) emask_k|=SSL_kDHr;

	if (dh_dsa) mask_k|=SSL_kDHd;
	if (dh_dsa_export) emask_k|=SSL_kDHd;

	if (rsa_enc || rsa_sign)
		{
		mask_a|=SSL_aRSA;
		emask_a|=SSL_aRSA;
		}

	if (dsa_sign)
		{
		mask_a|=SSL_aDSS;
		emask_a|=SSL_aDSS;
		}

	mask_a|=SSL_aNULL;
	emask_a|=SSL_aNULL;

#ifndef OPENSSL_NO_KRB5
	mask_k|=SSL_kKRB5;
	mask_a|=SSL_aKRB5;
	emask_k|=SSL_kKRB5;
	emask_a|=SSL_aKRB5;
#endif

	/* An ECC certificate may be usable for ECDH and/or
	 * ECDSA cipher suites depending on the key usage extension.
	 */
	if (have_ecc_cert)
		{
		/* This call populates extension flags (ex_flags) */
		x = (c->pkeys[SSL_PKEY_ECC]).x509;
		X509_check_purpose(x, -1, 0);
		ecdh_ok = (x->ex_flags & EXFLAG_KUSAGE) ?
		    (x->ex_kusage & X509v3_KU_KEY_AGREEMENT) : 1;
		ecdsa_ok = (x->ex_flags & EXFLAG_KUSAGE) ?
		    (x->ex_kusage & X509v3_KU_DIGITAL_SIGNATURE) : 1;
		ecc_pkey = X509_get_pubkey(x);
		ecc_pkey_size = (ecc_pkey != NULL) ?
		    EVP_PKEY_bits(ecc_pkey) : 0;
		EVP_PKEY_free(ecc_pkey);
		if ((x->sig_alg) && (x->sig_alg->algorithm))
			{
			signature_nid = OBJ_obj2nid(x->sig_alg->algorithm);
			OBJ_find_sigid_algs(signature_nid, &md_nid, &pk_nid);
			}
#ifndef OPENSSL_NO_ECDH
		if (ecdh_ok)
			{

			if (pk_nid == NID_rsaEncryption || pk_nid == NID_rsa)
				{
				mask_k|=SSL_kECDHr;
				mask_a|=SSL_aECDH;
				if (ecc_pkey_size <= 163)
					{
					emask_k|=SSL_kECDHr;
					emask_a|=SSL_aECDH;
					}
				}

			if (pk_nid == NID_X9_62_id_ecPublicKey)
				{
				mask_k|=SSL_kECDHe;
				mask_a|=SSL_aECDH;
				if (ecc_pkey_size <= 163)
					{
					emask_k|=SSL_kECDHe;
					emask_a|=SSL_aECDH;
					}
				}
			}
#endif
#ifndef OPENSSL_NO_ECDSA
		if (ecdsa_ok)
			{
			mask_a|=SSL_aECDSA;
			emask_a|=SSL_aECDSA;
			}
#endif
		}

#ifndef OPENSSL_NO_ECDH
	if (have_ecdh_tmp)
		{
		mask_k|=SSL_kEECDH;
		emask_k|=SSL_kEECDH;
		}
#endif

#ifndef OPENSSL_NO_PSK
	mask_k |= SSL_kPSK;
	mask_a |= SSL_aPSK;
	emask_k |= SSL_kPSK;
	emask_a |= SSL_aPSK;
#endif

	c->mask_k=mask_k;
	c->mask_a=mask_a;
	c->export_mask_k=emask_k;
	c->export_mask_a=emask_a;
	c->valid=1;
	}