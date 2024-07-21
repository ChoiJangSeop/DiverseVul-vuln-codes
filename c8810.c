static int mod_process(void *arg, eap_handler_t *handler)
{
	pwd_session_t *session;
	pwd_hdr *hdr;
	pwd_id_packet_t *packet;
	eap_packet_t *response;
	REQUEST *request, *fake;
	VALUE_PAIR *pw, *vp;
	EAP_DS *eap_ds;
	size_t in_len;
	int ret = 0;
	eap_pwd_t *inst = (eap_pwd_t *)arg;
	uint16_t offset;
	uint8_t exch, *in, *ptr, msk[MSK_EMSK_LEN], emsk[MSK_EMSK_LEN];
	uint8_t peer_confirm[SHA256_DIGEST_LENGTH];

	if (((eap_ds = handler->eap_ds) == NULL) || !inst) return 0;

	session = (pwd_session_t *)handler->opaque;
	request = handler->request;
	response = handler->eap_ds->response;
	hdr = (pwd_hdr *)response->type.data;

	/*
	 *	The header must be at least one byte.
	 */
	if (!hdr || (response->type.length < sizeof(pwd_hdr))) {
		RDEBUG("Packet with insufficient data");
		return 0;
	}

	in = hdr->data;
	in_len = response->type.length - sizeof(pwd_hdr);

	/*
	* see if we're fragmenting, if so continue until we're done
	*/
	if (session->out_pos) {
		if (in_len) RDEBUG2("pwd got something more than an ACK for a fragment");

		return send_pwd_request(session, eap_ds);
	}

	/*
	* the first fragment will have a total length, make a
	* buffer to hold all the fragments
	*/
	if (EAP_PWD_GET_LENGTH_BIT(hdr)) {
		if (session->in) {
			RDEBUG2("pwd already alloced buffer for fragments");
			return 0;
		}

		if (in_len < 2) {
			RDEBUG("Invalid packet: length bit set, but no length field");
			return 0;
		}

		session->in_len = ntohs(in[0] * 256 | in[1]);
		if ((session->in = talloc_zero_array(session, uint8_t, session->in_len)) == NULL) {
			RDEBUG2("pwd cannot allocate %zd buffer to hold fragments",
				session->in_len);
			return 0;
		}
		memset(session->in, 0, session->in_len);
		session->in_pos = 0;
		in += sizeof(uint16_t);
		in_len -= sizeof(uint16_t);
	}

	/*
	 * all fragments, including the 1st will have the M(ore) bit set,
	 * buffer those fragments!
	 */
	if (EAP_PWD_GET_MORE_BIT(hdr)) {
		if (!session->in) {
			RDEBUG2("Unexpected fragment.");
			return 0;
		}

		if ((session->in_pos + in_len) > session->in_len) {
			RDEBUG2("Fragment overflows packet.");
			return 0;
		}

		memcpy(session->in + session->in_pos, in, in_len);
		session->in_pos += in_len;

		/*
		 * send back an ACK for this fragment
		 */
		exch = EAP_PWD_GET_EXCHANGE(hdr);
		eap_ds->request->code = PW_EAP_REQUEST;
		eap_ds->request->type.num = PW_EAP_PWD;
		eap_ds->request->type.length = sizeof(pwd_hdr);
		if ((eap_ds->request->type.data = talloc_array(eap_ds->request, uint8_t, sizeof(pwd_hdr))) == NULL) {
			return 0;
		}
		hdr = (pwd_hdr *)eap_ds->request->type.data;
		EAP_PWD_SET_EXCHANGE(hdr, exch);
		return 1;
	}


	if (session->in) {
		/*
		 * the last fragment...
		 */
		if ((session->in_pos + in_len) > session->in_len) {
			RDEBUG2("pwd will not overflow a fragment buffer. Nope, not prudent");
			return 0;
		}
		memcpy(session->in + session->in_pos, in, in_len);
		in = session->in;
		in_len = session->in_len;
	}

	switch (session->state) {
	case PWD_STATE_ID_REQ:
	{
		BIGNUM	*x = NULL, *y = NULL;

		if (EAP_PWD_GET_EXCHANGE(hdr) != EAP_PWD_EXCH_ID) {
			RDEBUG2("pwd exchange is incorrect: not ID");
			return 0;
		}

		packet = (pwd_id_packet_t *) in;
		if (in_len < sizeof(*packet)) {
			RDEBUG("Packet is too small (%zd < %zd).", in_len, sizeof(*packet));
			return 0;
		}

		if ((packet->prf != EAP_PWD_DEF_PRF) ||
		    (packet->random_function != EAP_PWD_DEF_RAND_FUN) ||
		    (packet->prep != EAP_PWD_PREP_NONE) ||
		    (CRYPTO_memcmp(packet->token, &session->token, 4)) ||
		    (packet->group_num != ntohs(session->group_num))) {
			RDEBUG2("pwd id response is invalid");
			return 0;
		}
		/*
		 * we've agreed on the ciphersuite, record it...
		 */
		ptr = (uint8_t *)&session->ciphersuite;
		memcpy(ptr, (char *)&packet->group_num, sizeof(uint16_t));
		ptr += sizeof(uint16_t);
		*ptr = EAP_PWD_DEF_RAND_FUN;
		ptr += sizeof(uint8_t);
		*ptr = EAP_PWD_DEF_PRF;

		session->peer_id_len = in_len - sizeof(pwd_id_packet_t);
		if (session->peer_id_len >= sizeof(session->peer_id)) {
			RDEBUG2("pwd id response is malformed");
			return 0;
		}

		memcpy(session->peer_id, packet->identity, session->peer_id_len);
		session->peer_id[session->peer_id_len] = '\0';

		/*
		 * make fake request to get the password for the usable ID
		 */
		if ((fake = request_alloc_fake(handler->request)) == NULL) {
			RDEBUG("pwd unable to create fake request!");
			return 0;
		}
		fake->username = fr_pair_afrom_num(fake->packet, PW_USER_NAME, 0);
		if (!fake->username) {
			RDEBUG("Failed creating pair for peer id");
			talloc_free(fake);
			return 0;
		}
		fr_pair_value_bstrncpy(fake->username, session->peer_id, session->peer_id_len);
		fr_pair_add(&fake->packet->vps, fake->username);

		if ((vp = fr_pair_find_by_num(request->config, PW_VIRTUAL_SERVER, 0, TAG_ANY)) != NULL) {
			fake->server = vp->vp_strvalue;
		} else if (inst->virtual_server) {
			fake->server = inst->virtual_server;
		} /* else fake->server == request->server */

		RDEBUG("Sending tunneled request");
		rdebug_pair_list(L_DBG_LVL_1, request, fake->packet->vps, NULL);

		if (fake->server) {
			RDEBUG("server %s {", fake->server);
		} else {
			RDEBUG("server {");
		}

		/*
		 *	Call authorization recursively, which will
		 *	get the password.
		 */
		RINDENT();
		process_authorize(0, fake);
		REXDENT();

		/*
		 *	Note that we don't do *anything* with the reply
		 *	attributes.
		 */
		if (fake->server) {
			RDEBUG("} # server %s", fake->server);
		} else {
			RDEBUG("}");
		}

		RDEBUG("Got tunneled reply code %d", fake->reply->code);
		rdebug_pair_list(L_DBG_LVL_1, request, fake->reply->vps, NULL);

		if ((pw = fr_pair_find_by_num(fake->config, PW_CLEARTEXT_PASSWORD, 0, TAG_ANY)) == NULL) {
			DEBUG2("failed to find password for %s to do pwd authentication",
			session->peer_id);
			talloc_free(fake);
			return 0;
		}

		if (compute_password_element(session, session->group_num,
			     		     pw->data.strvalue, strlen(pw->data.strvalue),
					     inst->server_id, strlen(inst->server_id),
					     session->peer_id, strlen(session->peer_id),
					     &session->token)) {
			DEBUG2("failed to obtain password element");
			talloc_free(fake);
			return 0;
		}
		TALLOC_FREE(fake);

		/*
		 * compute our scalar and element
		 */
		if (compute_scalar_element(session, inst->bnctx)) {
			DEBUG2("failed to compute server's scalar and element");
			return 0;
		}

		MEM(x = BN_new());
		MEM(y = BN_new());

		/*
		 * element is a point, get both coordinates: x and y
		 */
		if (!EC_POINT_get_affine_coordinates_GFp(session->group, session->my_element, x, y,
							 inst->bnctx)) {
			DEBUG2("server point assignment failed");
			BN_clear_free(x);
			BN_clear_free(y);
			return 0;
		}

		/*
		 * construct request
		 */
		session->out_len = BN_num_bytes(session->order) + (2 * BN_num_bytes(session->prime));
		if ((session->out = talloc_array(session, uint8_t, session->out_len)) == NULL) {
			return 0;
		}
		memset(session->out, 0, session->out_len);

		ptr = session->out;
		offset = BN_num_bytes(session->prime) - BN_num_bytes(x);
		BN_bn2bin(x, ptr + offset);
		BN_clear_free(x);

		ptr += BN_num_bytes(session->prime);
		offset = BN_num_bytes(session->prime) - BN_num_bytes(y);
		BN_bn2bin(y, ptr + offset);
		BN_clear_free(y);

		ptr += BN_num_bytes(session->prime);
		offset = BN_num_bytes(session->order) - BN_num_bytes(session->my_scalar);
		BN_bn2bin(session->my_scalar, ptr + offset);

		session->state = PWD_STATE_COMMIT;
		ret = send_pwd_request(session, eap_ds);
	}
		break;

		case PWD_STATE_COMMIT:
		if (EAP_PWD_GET_EXCHANGE(hdr) != EAP_PWD_EXCH_COMMIT) {
			RDEBUG2("pwd exchange is incorrect: not commit!");
			return 0;
		}

		/*
		 * process the peer's commit and generate the shared key, k
		 */
		if (process_peer_commit(session, in, in_len, inst->bnctx)) {
			RDEBUG2("failed to process peer's commit");
			return 0;
		}

		/*
		 * compute our confirm blob
		 */
		if (compute_server_confirm(session, session->my_confirm, inst->bnctx)) {
			ERROR("rlm_eap_pwd: failed to compute confirm!");
			return 0;
		}

		/*
		 * construct a response...which is just our confirm blob
		 */
		session->out_len = SHA256_DIGEST_LENGTH;
		if ((session->out = talloc_array(session, uint8_t, session->out_len)) == NULL) {
			return 0;
		}

		memset(session->out, 0, session->out_len);
		memcpy(session->out, session->my_confirm, SHA256_DIGEST_LENGTH);

		session->state = PWD_STATE_CONFIRM;
		ret = send_pwd_request(session, eap_ds);
		break;

	case PWD_STATE_CONFIRM:
		if (in_len < SHA256_DIGEST_LENGTH) {
			RDEBUG("Peer confirm is too short (%zd < %d)",
			       in_len, SHA256_DIGEST_LENGTH);
			return 0;
		}

		if (EAP_PWD_GET_EXCHANGE(hdr) != EAP_PWD_EXCH_CONFIRM) {
			RDEBUG2("pwd exchange is incorrect: not commit!");
			return 0;
		}
		if (compute_peer_confirm(session, peer_confirm, inst->bnctx)) {
			RDEBUG2("pwd exchange cannot compute peer's confirm");
			return 0;
		}
		if (CRYPTO_memcmp(peer_confirm, in, SHA256_DIGEST_LENGTH)) {
			RDEBUG2("pwd exchange fails: peer confirm is incorrect!");
			return 0;
		}
		if (compute_keys(session, peer_confirm, msk, emsk)) {
			RDEBUG2("pwd exchange cannot generate (E)MSK!");
			return 0;
		}
		eap_ds->request->code = PW_EAP_SUCCESS;
		/*
		 * return the MSK (in halves)
		 */
		eap_add_reply(handler->request, "MS-MPPE-Recv-Key", msk, MPPE_KEY_LEN);
		eap_add_reply(handler->request, "MS-MPPE-Send-Key", msk + MPPE_KEY_LEN, MPPE_KEY_LEN);

		ret = 1;
		break;

	default:
		RDEBUG2("unknown PWD state");
		return 0;
	}

	/*
	 * we processed the buffered fragments, get rid of them
	 */
	if (session->in) {
		talloc_free(session->in);
		session->in = NULL;
	}

	return ret;
}