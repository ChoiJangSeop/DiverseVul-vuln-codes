eap_response(esp, inp, id, len)
eap_state *esp;
u_char *inp;
int id;
int len;
{
	u_char typenum;
	u_char vallen;
	int secret_len;
	char secret[MAXSECRETLEN];
	char rhostname[256];
	MD5_CTX mdContext;
	u_char hash[MD5_SIGNATURE_SIZE];
#ifdef USE_SRP
	struct t_server *ts;
	struct t_num A;
	SHA1_CTX ctxt;
	u_char dig[SHA_DIGESTSIZE];
#endif /* USE_SRP */

	if (esp->es_server.ea_id != id) {
		dbglog("EAP: discarding Response %d; expected ID %d", id,
		    esp->es_server.ea_id);
		return;
	}

	esp->es_server.ea_responses++;

	if (len <= 0) {
		error("EAP: empty Response message discarded");
		return;
	}

	GETCHAR(typenum, inp);
	len--;

	switch (typenum) {
	case EAPT_IDENTITY:
		if (esp->es_server.ea_state != eapIdentify) {
			dbglog("EAP discarding unwanted Identify \"%.q\"", len,
			    inp);
			break;
		}
		info("EAP: unauthenticated peer name \"%.*q\"", len, inp);
		if (esp->es_server.ea_peer != NULL &&
		    esp->es_server.ea_peer != remote_name)
			free(esp->es_server.ea_peer);
		esp->es_server.ea_peer = malloc(len + 1);
		if (esp->es_server.ea_peer == NULL) {
			esp->es_server.ea_peerlen = 0;
			eap_figure_next_state(esp, 1);
			break;
		}
		BCOPY(inp, esp->es_server.ea_peer, len);
		esp->es_server.ea_peer[len] = '\0';
		esp->es_server.ea_peerlen = len;
		eap_figure_next_state(esp, 0);
		break;

	case EAPT_NOTIFICATION:
		dbglog("EAP unexpected Notification; response discarded");
		break;

	case EAPT_NAK:
		if (len < 1) {
			info("EAP: Nak Response with no suggested protocol");
			eap_figure_next_state(esp, 1);
			break;
		}

		GETCHAR(vallen, inp);
		len--;

		if (!explicit_remote && esp->es_server.ea_state == eapIdentify){
			/* Peer cannot Nak Identify Request */
			eap_figure_next_state(esp, 1);
			break;
		}

		switch (vallen) {
		case EAPT_SRP:
			/* Run through SRP validator selection again. */
			esp->es_server.ea_state = eapIdentify;
			eap_figure_next_state(esp, 0);
			break;

		case EAPT_MD5CHAP:
			esp->es_server.ea_state = eapMD5Chall;
			break;

		default:
			dbglog("EAP: peer requesting unknown Type %d", vallen);
			switch (esp->es_server.ea_state) {
			case eapSRP1:
			case eapSRP2:
			case eapSRP3:
				esp->es_server.ea_state = eapMD5Chall;
				break;
			case eapMD5Chall:
			case eapSRP4:
				esp->es_server.ea_state = eapIdentify;
				eap_figure_next_state(esp, 0);
				break;
			default:
				break;
			}
			break;
		}
		break;

	case EAPT_MD5CHAP:
		if (esp->es_server.ea_state != eapMD5Chall) {
			error("EAP: unexpected MD5-Response");
			eap_figure_next_state(esp, 1);
			break;
		}
		if (len < 1) {
			error("EAP: received MD5-Response with no data");
			eap_figure_next_state(esp, 1);
			break;
		}
		GETCHAR(vallen, inp);
		len--;
		if (vallen != 16 || vallen > len) {
			error("EAP: MD5-Response with bad length %d", vallen);
			eap_figure_next_state(esp, 1);
			break;
		}

		/* Not so likely to happen. */
		if (vallen >= len + sizeof (rhostname)) {
			dbglog("EAP: trimming really long peer name down");
			BCOPY(inp + vallen, rhostname, sizeof (rhostname) - 1);
			rhostname[sizeof (rhostname) - 1] = '\0';
		} else {
			BCOPY(inp + vallen, rhostname, len - vallen);
			rhostname[len - vallen] = '\0';
		}

		/* In case the remote doesn't give us his name. */
		if (explicit_remote ||
		    (remote_name[0] != '\0' && vallen == len))
			strlcpy(rhostname, remote_name, sizeof (rhostname));

		/*
		 * Get the secret for authenticating the specified
		 * host.
		 */
		if (!get_secret(esp->es_unit, rhostname,
		    esp->es_server.ea_name, secret, &secret_len, 1)) {
			dbglog("EAP: no MD5 secret for auth of %q", rhostname);
			eap_send_failure(esp);
			break;
		}
		MD5_Init(&mdContext);
		MD5_Update(&mdContext, &esp->es_server.ea_id, 1);
		MD5_Update(&mdContext, (u_char *)secret, secret_len);
		BZERO(secret, sizeof (secret));
		MD5_Update(&mdContext, esp->es_challenge, esp->es_challen);
		MD5_Final(hash, &mdContext);
		if (BCMP(hash, inp, MD5_SIGNATURE_SIZE) != 0) {
			eap_send_failure(esp);
			break;
		}
		esp->es_server.ea_type = EAPT_MD5CHAP;
		eap_send_success(esp);
		eap_figure_next_state(esp, 0);
		if (esp->es_rechallenge != 0)
			TIMEOUT(eap_rechallenge, esp, esp->es_rechallenge);
		break;

#ifdef USE_SRP
	case EAPT_SRP:
		if (len < 1) {
			error("EAP: empty SRP Response");
			eap_figure_next_state(esp, 1);
			break;
		}
		GETCHAR(typenum, inp);
		len--;
		switch (typenum) {
		case EAPSRP_CKEY:
			if (esp->es_server.ea_state != eapSRP1) {
				error("EAP: unexpected SRP Subtype 1 Response");
				eap_figure_next_state(esp, 1);
				break;
			}
			A.data = inp;
			A.len = len;
			ts = (struct t_server *)esp->es_server.ea_session;
			assert(ts != NULL);
			esp->es_server.ea_skey = t_servergetkey(ts, &A);
			if (esp->es_server.ea_skey == NULL) {
				/* Client's A value is bogus; terminate now */
				error("EAP: bogus A value from client");
				eap_send_failure(esp);
			} else {
				eap_figure_next_state(esp, 0);
			}
			break;

		case EAPSRP_CVALIDATOR:
			if (esp->es_server.ea_state != eapSRP2) {
				error("EAP: unexpected SRP Subtype 2 Response");
				eap_figure_next_state(esp, 1);
				break;
			}
			if (len < sizeof (u_int32_t) + SHA_DIGESTSIZE) {
				error("EAP: M1 length %d < %d", len,
				    sizeof (u_int32_t) + SHA_DIGESTSIZE);
				eap_figure_next_state(esp, 1);
				break;
			}
			GETLONG(esp->es_server.ea_keyflags, inp);
			ts = (struct t_server *)esp->es_server.ea_session;
			assert(ts != NULL);
			if (t_serververify(ts, inp)) {
				info("EAP: unable to validate client identity");
				eap_send_failure(esp);
				break;
			}
			eap_figure_next_state(esp, 0);
			break;

		case EAPSRP_ACK:
			if (esp->es_server.ea_state != eapSRP3) {
				error("EAP: unexpected SRP Subtype 3 Response");
				eap_send_failure(esp);
				break;
			}
			esp->es_server.ea_type = EAPT_SRP;
			eap_send_success(esp);
			eap_figure_next_state(esp, 0);
			if (esp->es_rechallenge != 0)
				TIMEOUT(eap_rechallenge, esp,
				    esp->es_rechallenge);
			if (esp->es_lwrechallenge != 0)
				TIMEOUT(srp_lwrechallenge, esp,
				    esp->es_lwrechallenge);
			break;

		case EAPSRP_LWRECHALLENGE:
			if (esp->es_server.ea_state != eapSRP4) {
				info("EAP: unexpected SRP Subtype 4 Response");
				return;
			}
			if (len != SHA_DIGESTSIZE) {
				error("EAP: bad Lightweight rechallenge "
				    "response");
				return;
			}
			SHA1Init(&ctxt);
			vallen = id;
			SHA1Update(&ctxt, &vallen, 1);
			SHA1Update(&ctxt, esp->es_server.ea_skey,
			    SESSION_KEY_LEN);
			SHA1Update(&ctxt, esp->es_challenge, esp->es_challen);
			SHA1Update(&ctxt, esp->es_server.ea_peer,
			    esp->es_server.ea_peerlen);
			SHA1Final(dig, &ctxt);
			if (BCMP(dig, inp, SHA_DIGESTSIZE) != 0) {
				error("EAP: failed Lightweight rechallenge");
				eap_send_failure(esp);
				break;
			}
			esp->es_server.ea_state = eapOpen;
			if (esp->es_lwrechallenge != 0)
				TIMEOUT(srp_lwrechallenge, esp,
				    esp->es_lwrechallenge);
			break;
		}
		break;
#endif /* USE_SRP */

	default:
		/* This can't happen. */
		error("EAP: unknown Response type %d; ignored", typenum);
		return;
	}

	if (esp->es_server.ea_timeout > 0) {
		UNTIMEOUT(eap_server_timeout, (void *)esp);
	}

	if (esp->es_server.ea_state != eapBadAuth &&
	    esp->es_server.ea_state != eapOpen) {
		esp->es_server.ea_id++;
		eap_send_request(esp);
	}
}