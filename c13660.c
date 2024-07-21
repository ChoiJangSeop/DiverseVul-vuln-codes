static __u8 *nci_extract_rf_params_nfcb_passive_poll(struct nci_dev *ndev,
			struct rf_tech_specific_params_nfcb_poll *nfcb_poll,
						     __u8 *data)
{
	nfcb_poll->sensb_res_len = *data++;

	pr_debug("sensb_res_len %d\n", nfcb_poll->sensb_res_len);

	memcpy(nfcb_poll->sensb_res, data, nfcb_poll->sensb_res_len);
	data += nfcb_poll->sensb_res_len;

	return data;
}