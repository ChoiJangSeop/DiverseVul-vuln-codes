read_fru_area_section(struct ipmi_intf * intf, struct fru_info *fru, uint8_t id,
			uint32_t offset, uint32_t length, uint8_t *frubuf)
{
	static uint32_t fru_data_rqst_size = 20;
	uint32_t off = offset, tmp, finish;
	struct ipmi_rs * rsp;
	struct ipmi_rq req;
	uint8_t msg_data[4];

	if (offset > fru->size) {
		lprintf(LOG_ERR, "Read FRU Area offset incorrect: %d > %d",
			offset, fru->size);
		return -1;
	}

	finish = offset + length;
	if (finish > fru->size) {
		finish = fru->size;
		lprintf(LOG_NOTICE, "Read FRU Area length %d too large, "
			"Adjusting to %d",
			offset + length, finish - offset);
	}

	memset(&req, 0, sizeof(req));
	req.msg.netfn = IPMI_NETFN_STORAGE;
	req.msg.cmd = GET_FRU_DATA;
	req.msg.data = msg_data;
	req.msg.data_len = 4;

#ifdef LIMIT_ALL_REQUEST_SIZE
	if (fru_data_rqst_size > 16)
#else
	if (fru->access && fru_data_rqst_size > 16)
#endif
		fru_data_rqst_size = 16;
	do {
		tmp = fru->access ? off >> 1 : off;
		msg_data[0] = id;
		msg_data[1] = (uint8_t)(tmp & 0xff);
		msg_data[2] = (uint8_t)(tmp >> 8);
		tmp = finish - off;
		if (tmp > fru_data_rqst_size)
			msg_data[3] = (uint8_t)fru_data_rqst_size;
		else
			msg_data[3] = (uint8_t)tmp;

		rsp = intf->sendrecv(intf, &req);
		if (!rsp) {
			lprintf(LOG_NOTICE, "FRU Read failed");
			break;
		}
		if (rsp->ccode) {
			/* if we get C7 or C8  or CA return code then we requested too
			* many bytes at once so try again with smaller size */
			if (fru_cc_rq2big(rsp->ccode) && (--fru_data_rqst_size > FRU_BLOCK_SZ)) {
				lprintf(LOG_INFO,
					"Retrying FRU read with request size %d",
					fru_data_rqst_size);
				continue;
			}
			lprintf(LOG_NOTICE, "FRU Read failed: %s",
				val2str(rsp->ccode, completion_code_vals));
			break;
		}

		tmp = fru->access ? rsp->data[0] << 1 : rsp->data[0];
		memcpy((frubuf + off)-offset, rsp->data + 1, tmp);
		off += tmp;

		/* sometimes the size returned in the Info command
		* is too large.  return 0 so higher level function
		* still attempts to parse what was returned */
		if (tmp == 0 && off < finish)
			return 0;

	} while (off < finish);

	if (off < finish)
		return -1;

	return 0;
}