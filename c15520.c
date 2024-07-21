static size_t handle_vendordep_pdu(struct avctp *conn, uint8_t transaction,
					uint8_t *code, uint8_t *subunit,
					uint8_t *operands, size_t operand_count,
					void *user_data)
{
	struct avrcp *session = user_data;
	const struct control_pdu_handler *handler;
	struct avrcp_header *pdu = (void *) operands;
	uint32_t company_id = get_company_id(pdu->company_id);

	if (company_id != IEEEID_BTSIG) {
		*code = AVC_CTYPE_NOT_IMPLEMENTED;
		return 0;
	}

	DBG("AVRCP PDU 0x%02X, company 0x%06X len 0x%04X",
			pdu->pdu_id, company_id, ntohs(pdu->params_len));

	pdu->packet_type = 0;
	pdu->rsvd = 0;

	if (operand_count < AVRCP_HEADER_LENGTH) {
		pdu->params[0] = AVRCP_STATUS_INVALID_COMMAND;
		goto err_metadata;
	}

	for (handler = session->control_handlers; handler->pdu_id; handler++) {
		if (handler->pdu_id == pdu->pdu_id)
			break;
	}

	if (handler->pdu_id != pdu->pdu_id || handler->code != *code) {
		pdu->params[0] = AVRCP_STATUS_INVALID_COMMAND;
		goto err_metadata;
	}

	if (!handler->func) {
		pdu->params[0] = AVRCP_STATUS_INVALID_PARAM;
		goto err_metadata;
	}

	*code = handler->func(session, pdu, transaction);

	if (*code != AVC_CTYPE_REJECTED &&
				pdu->pdu_id != AVRCP_GET_ELEMENT_ATTRIBUTES &&
				pdu->pdu_id != AVRCP_REQUEST_CONTINUING &&
				pdu->pdu_id != AVRCP_ABORT_CONTINUING)
		session_abort_pending_pdu(session);

	return AVRCP_HEADER_LENGTH + ntohs(pdu->params_len);

err_metadata:
	pdu->params_len = htons(1);
	*code = AVC_CTYPE_REJECTED;

	return AVRCP_HEADER_LENGTH + 1;
}