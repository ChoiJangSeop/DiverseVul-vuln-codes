kvp_respond_to_host(char *key, char *value, int error)
{
	struct hv_kvp_msg  *kvp_msg;
	struct hv_kvp_msg_enumerate  *kvp_data;
	char	*key_name;
	struct icmsg_hdr *icmsghdrp;
	int	keylen, valuelen;
	u32	buf_len;
	struct vmbus_channel *channel;
	u64	req_id;

	/*
	 * If a transaction is not active; log and return.
	 */

	if (!kvp_transaction.active) {
		/*
		 * This is a spurious call!
		 */
		pr_warn("KVP: Transaction not active\n");
		return;
	}
	/*
	 * Copy the global state for completing the transaction. Note that
	 * only one transaction can be active at a time.
	 */

	buf_len = kvp_transaction.recv_len;
	channel = kvp_transaction.recv_channel;
	req_id = kvp_transaction.recv_req_id;

	kvp_transaction.active = false;

	if (channel->onchannel_callback == NULL)
		/*
		 * We have raced with util driver being unloaded;
		 * silently return.
		 */
		return;

	icmsghdrp = (struct icmsg_hdr *)
			&recv_buffer[sizeof(struct vmbuspipe_hdr)];
	kvp_msg = (struct hv_kvp_msg *)
			&recv_buffer[sizeof(struct vmbuspipe_hdr) +
			sizeof(struct icmsg_hdr)];
	kvp_data = &kvp_msg->kvp_data;
	key_name = key;

	/*
	 * If the error parameter is set, terminate the host's enumeration.
	 */
	if (error) {
		/*
		 * We don't support this index or the we have timedout;
		 * terminate the host-side iteration by returning an error.
		 */
		icmsghdrp->status = HV_E_FAIL;
		goto response_done;
	}

	/*
	 * The windows host expects the key/value pair to be encoded
	 * in utf16.
	 */
	keylen = utf8s_to_utf16s(key_name, strlen(key_name),
				(wchar_t *)kvp_data->data.key);
	kvp_data->data.key_size = 2*(keylen + 1); /* utf16 encoding */
	valuelen = utf8s_to_utf16s(value, strlen(value),
				(wchar_t *)kvp_data->data.value);
	kvp_data->data.value_size = 2*(valuelen + 1); /* utf16 encoding */

	kvp_data->data.value_type = REG_SZ; /* all our values are strings */
	icmsghdrp->status = HV_S_OK;

response_done:
	icmsghdrp->icflags = ICMSGHDRFLAG_TRANSACTION | ICMSGHDRFLAG_RESPONSE;

	vmbus_sendpacket(channel, recv_buffer, buf_len, req_id,
				VM_PKT_DATA_INBAND, 0);

}