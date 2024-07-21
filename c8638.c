eap_request(esp, inp, id, len)
eap_state *esp;
u_char *inp;
int id;
int len;
{
	u_char typenum;
	u_char vallen;
	int secret_len;
	char secret[MAXWORDLEN];
	char rhostname[256];
	MD5_CTX mdContext;
	u_char hash[MD5_SIGNATURE_SIZE];
#ifdef USE_SRP
	struct t_client *tc;
	struct t_num sval, gval, Nval, *Ap, Bval;
	u_char vals[2];
	SHA1_CTX ctxt;
	u_char dig[SHA_DIGESTSIZE];
	int fd;
#endif /* USE_SRP */

	/*
	 * Note: we update es_client.ea_id *only if* a Response
	 * message is being generated.  Otherwise, we leave it the
	 * same for duplicate detection purposes.
	 */

	esp->es_client.ea_requests++;
	if (esp->es_client.ea_maxrequests != 0 &&
	    esp->es_client.ea_requests > esp->es_client.ea_maxrequests) {
		info("EAP: received too many Request messages");
		if (esp->es_client.ea_timeout > 0) {
			UNTIMEOUT(eap_client_timeout, (void *)esp);
		}
		auth_withpeer_fail(esp->es_unit, PPP_EAP);
		return;
	}

	if (len <= 0) {
		error("EAP: empty Request message discarded");
		return;
	}

	GETCHAR(typenum, inp);
	len--;

	switch (typenum) {
	case EAPT_IDENTITY:
		if (len > 0)
			info("EAP: Identity prompt \"%.*q\"", len, inp);
#ifdef USE_SRP
		if (esp->es_usepseudo &&
		    (esp->es_usedpseudo == 0 ||
			(esp->es_usedpseudo == 1 &&
			    id == esp->es_client.ea_id))) {
			esp->es_usedpseudo = 1;
			/* Try to get a pseudonym */
			if ((fd = open_pn_file(O_RDONLY)) >= 0) {
				strcpy(rhostname, SRP_PSEUDO_ID);
				len = read(fd, rhostname + SRP_PSEUDO_LEN,
				    sizeof (rhostname) - SRP_PSEUDO_LEN);
				/* XXX NAI unsupported */
				if (len > 0) {
					eap_send_response(esp, id, typenum,
					    rhostname, len + SRP_PSEUDO_LEN);
				}
				(void) close(fd);
				if (len > 0)
					break;
			}
		}
		/* Stop using pseudonym now. */
		if (esp->es_usepseudo && esp->es_usedpseudo != 2) {
			remove_pn_file();
			esp->es_usedpseudo = 2;
		}
#endif /* USE_SRP */
		eap_send_response(esp, id, typenum, esp->es_client.ea_name,
		    esp->es_client.ea_namelen);
		break;

	case EAPT_NOTIFICATION:
		if (len > 0)
			info("EAP: Notification \"%.*q\"", len, inp);
		eap_send_response(esp, id, typenum, NULL, 0);
		break;

	case EAPT_NAK:
		/*
		 * Avoid the temptation to send Response Nak in reply
		 * to Request Nak here.  It can only lead to trouble.
		 */
		warn("EAP: unexpected Nak in Request; ignored");
		/* Return because we're waiting for something real. */
		return;

	case EAPT_MD5CHAP:
		if (len < 1) {
			error("EAP: received MD5-Challenge with no data");
			/* Bogus request; wait for something real. */
			return;
		}
		GETCHAR(vallen, inp);
		len--;
		if (vallen < 8 || vallen > len) {
			error("EAP: MD5-Challenge with bad length %d (8..%d)",
			    vallen, len);
			/* Try something better. */
			eap_send_nak(esp, id, EAPT_SRP);
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
		 * Get the secret for authenticating ourselves with
		 * the specified host.
		 */
		if (!get_secret(esp->es_unit, esp->es_client.ea_name,
		    rhostname, secret, &secret_len, 0)) {
			dbglog("EAP: no MD5 secret for auth to %q", rhostname);
			eap_send_nak(esp, id, EAPT_SRP);
			break;
		}
		MD5_Init(&mdContext);
		typenum = id;
		MD5_Update(&mdContext, &typenum, 1);
		MD5_Update(&mdContext, (u_char *)secret, secret_len);
		BZERO(secret, sizeof (secret));
		MD5_Update(&mdContext, inp, vallen);
		MD5_Final(hash, &mdContext);
		eap_chap_response(esp, id, hash, esp->es_client.ea_name,
		    esp->es_client.ea_namelen);
		break;

#ifdef USE_SRP
	case EAPT_SRP:
		if (len < 1) {
			error("EAP: received empty SRP Request");
			/* Bogus request; wait for something real. */
			return;
		}

		/* Get subtype */
		GETCHAR(vallen, inp);
		len--;
		switch (vallen) {
		case EAPSRP_CHALLENGE:
			tc = NULL;
			if (esp->es_client.ea_session != NULL) {
				tc = (struct t_client *)esp->es_client.
				    ea_session;
				/*
				 * If this is a new challenge, then start
				 * over with a new client session context.
				 * Otherwise, just resend last response.
				 */
				if (id != esp->es_client.ea_id) {
					t_clientclose(tc);
					esp->es_client.ea_session = NULL;
					tc = NULL;
				}
			}
			/* No session key just yet */
			esp->es_client.ea_skey = NULL;
			if (tc == NULL) {
				GETCHAR(vallen, inp);
				len--;
				if (vallen >= len) {
					error("EAP: badly-formed SRP Challenge"
					    " (name)");
					/* Ignore badly-formed messages */
					return;
				}
				BCOPY(inp, rhostname, vallen);
				rhostname[vallen] = '\0';
				INCPTR(vallen, inp);
				len -= vallen;

				/*
				 * In case the remote doesn't give us his name,
				 * use configured name.
				 */
				if (explicit_remote ||
				    (remote_name[0] != '\0' && vallen == 0)) {
					strlcpy(rhostname, remote_name,
					    sizeof (rhostname));
				}

				if (esp->es_client.ea_peer != NULL)
					free(esp->es_client.ea_peer);
				esp->es_client.ea_peer = strdup(rhostname);
				esp->es_client.ea_peerlen = strlen(rhostname);

				GETCHAR(vallen, inp);
				len--;
				if (vallen >= len) {
					error("EAP: badly-formed SRP Challenge"
					    " (s)");
					/* Ignore badly-formed messages */
					return;
				}
				sval.data = inp;
				sval.len = vallen;
				INCPTR(vallen, inp);
				len -= vallen;

				GETCHAR(vallen, inp);
				len--;
				if (vallen > len) {
					error("EAP: badly-formed SRP Challenge"
					    " (g)");
					/* Ignore badly-formed messages */
					return;
				}
				/* If no generator present, then use value 2 */
				if (vallen == 0) {
					gval.data = (u_char *)"\002";
					gval.len = 1;
				} else {
					gval.data = inp;
					gval.len = vallen;
				}
				INCPTR(vallen, inp);
				len -= vallen;

				/*
				 * If no modulus present, then use well-known
				 * value.
				 */
				if (len == 0) {
					Nval.data = (u_char *)wkmodulus;
					Nval.len = sizeof (wkmodulus);
				} else {
					Nval.data = inp;
					Nval.len = len;
				}
				tc = t_clientopen(esp->es_client.ea_name,
				    &Nval, &gval, &sval);
				if (tc == NULL) {
					eap_send_nak(esp, id, EAPT_MD5CHAP);
					break;
				}
				esp->es_client.ea_session = (void *)tc;

				/* Add Challenge ID & type to verifier */
				vals[0] = id;
				vals[1] = EAPT_SRP;
				t_clientaddexdata(tc, vals, 2);
			}
			Ap = t_clientgenexp(tc);
			eap_srp_response(esp, id, EAPSRP_CKEY, Ap->data,
			    Ap->len);
			break;

		case EAPSRP_SKEY:
			tc = (struct t_client *)esp->es_client.ea_session;
			if (tc == NULL) {
				warn("EAP: peer sent Subtype 2 without 1");
				eap_send_nak(esp, id, EAPT_MD5CHAP);
				break;
			}
			if (esp->es_client.ea_skey != NULL) {
				/*
				 * ID number should not change here.  Warn
				 * if it does (but otherwise ignore).
				 */
				if (id != esp->es_client.ea_id) {
					warn("EAP: ID changed from %d to %d "
					    "in SRP Subtype 2 rexmit",
					    esp->es_client.ea_id, id);
				}
			} else {
				if (get_srp_secret(esp->es_unit,
				    esp->es_client.ea_name,
				    esp->es_client.ea_peer, secret, 0) == 0) {
					/*
					 * Can't work with this peer because
					 * the secret is missing.  Just give
					 * up.
					 */
					eap_send_nak(esp, id, EAPT_MD5CHAP);
					break;
				}
				Bval.data = inp;
				Bval.len = len;
				t_clientpasswd(tc, secret);
				BZERO(secret, sizeof (secret));
				esp->es_client.ea_skey =
				    t_clientgetkey(tc, &Bval);
				if (esp->es_client.ea_skey == NULL) {
					/* Server is rogue; stop now */
					error("EAP: SRP server is rogue");
					goto client_failure;
				}
			}
			eap_srpval_response(esp, id, SRPVAL_EBIT,
			    t_clientresponse(tc));
			break;

		case EAPSRP_SVALIDATOR:
			tc = (struct t_client *)esp->es_client.ea_session;
			if (tc == NULL || esp->es_client.ea_skey == NULL) {
				warn("EAP: peer sent Subtype 3 without 1/2");
				eap_send_nak(esp, id, EAPT_MD5CHAP);
				break;
			}
			/*
			 * If we're already open, then this ought to be a
			 * duplicate.  Otherwise, check that the server is
			 * who we think it is.
			 */
			if (esp->es_client.ea_state == eapOpen) {
				if (id != esp->es_client.ea_id) {
					warn("EAP: ID changed from %d to %d "
					    "in SRP Subtype 3 rexmit",
					    esp->es_client.ea_id, id);
				}
			} else {
				len -= sizeof (u_int32_t) + SHA_DIGESTSIZE;
				if (len < 0 || t_clientverify(tc, inp +
					sizeof (u_int32_t)) != 0) {
					error("EAP: SRP server verification "
					    "failed");
					goto client_failure;
				}
				GETLONG(esp->es_client.ea_keyflags, inp);
				/* Save pseudonym if user wants it. */
				if (len > 0 && esp->es_usepseudo) {
					INCPTR(SHA_DIGESTSIZE, inp);
					write_pseudonym(esp, inp, len, id);
				}
			}
			/*
			 * We've verified our peer.  We're now mostly done,
			 * except for waiting on the regular EAP Success
			 * message.
			 */
			eap_srp_response(esp, id, EAPSRP_ACK, NULL, 0);
			break;

		case EAPSRP_LWRECHALLENGE:
			if (len < 4) {
				warn("EAP: malformed Lightweight rechallenge");
				return;
			}
			SHA1Init(&ctxt);
			vals[0] = id;
			SHA1Update(&ctxt, vals, 1);
			SHA1Update(&ctxt, esp->es_client.ea_skey,
			    SESSION_KEY_LEN);
			SHA1Update(&ctxt, inp, len);
			SHA1Update(&ctxt, esp->es_client.ea_name,
			    esp->es_client.ea_namelen);
			SHA1Final(dig, &ctxt);
			eap_srp_response(esp, id, EAPSRP_LWRECHALLENGE, dig,
			    SHA_DIGESTSIZE);
			break;

		default:
			error("EAP: unknown SRP Subtype %d", vallen);
			eap_send_nak(esp, id, EAPT_MD5CHAP);
			break;
		}
		break;
#endif /* USE_SRP */

	default:
		info("EAP: unknown authentication type %d; Naking", typenum);
		eap_send_nak(esp, id, EAPT_SRP);
		break;
	}

	if (esp->es_client.ea_timeout > 0) {
		UNTIMEOUT(eap_client_timeout, (void *)esp);
		TIMEOUT(eap_client_timeout, (void *)esp,
		    esp->es_client.ea_timeout);
	}
	return;

#ifdef USE_SRP
client_failure:
	esp->es_client.ea_state = eapBadAuth;
	if (esp->es_client.ea_timeout > 0) {
		UNTIMEOUT(eap_client_timeout, (void *)esp);
	}
	esp->es_client.ea_session = NULL;
	t_clientclose(tc);
	auth_withpeer_fail(esp->es_unit, PPP_EAP);
#endif /* USE_SRP */
}