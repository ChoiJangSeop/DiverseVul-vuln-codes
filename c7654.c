int hci_conn_security(struct hci_conn *conn, __u8 sec_level, __u8 auth_type,
		      bool initiator)
{
	BT_DBG("hcon %p", conn);

	if (conn->type == LE_LINK)
		return smp_conn_security(conn, sec_level);

	/* For sdp we don't need the link key. */
	if (sec_level == BT_SECURITY_SDP)
		return 1;

	/* For non 2.1 devices and low security level we don't need the link
	   key. */
	if (sec_level == BT_SECURITY_LOW && !hci_conn_ssp_enabled(conn))
		return 1;

	/* For other security levels we need the link key. */
	if (!test_bit(HCI_CONN_AUTH, &conn->flags))
		goto auth;

	/* An authenticated FIPS approved combination key has sufficient
	 * security for security level 4. */
	if (conn->key_type == HCI_LK_AUTH_COMBINATION_P256 &&
	    sec_level == BT_SECURITY_FIPS)
		goto encrypt;

	/* An authenticated combination key has sufficient security for
	   security level 3. */
	if ((conn->key_type == HCI_LK_AUTH_COMBINATION_P192 ||
	     conn->key_type == HCI_LK_AUTH_COMBINATION_P256) &&
	    sec_level == BT_SECURITY_HIGH)
		goto encrypt;

	/* An unauthenticated combination key has sufficient security for
	   security level 1 and 2. */
	if ((conn->key_type == HCI_LK_UNAUTH_COMBINATION_P192 ||
	     conn->key_type == HCI_LK_UNAUTH_COMBINATION_P256) &&
	    (sec_level == BT_SECURITY_MEDIUM || sec_level == BT_SECURITY_LOW))
		goto encrypt;

	/* A combination key has always sufficient security for the security
	   levels 1 or 2. High security level requires the combination key
	   is generated using maximum PIN code length (16).
	   For pre 2.1 units. */
	if (conn->key_type == HCI_LK_COMBINATION &&
	    (sec_level == BT_SECURITY_MEDIUM || sec_level == BT_SECURITY_LOW ||
	     conn->pin_length == 16))
		goto encrypt;

auth:
	if (test_bit(HCI_CONN_ENCRYPT_PEND, &conn->flags))
		return 0;

	if (initiator)
		set_bit(HCI_CONN_AUTH_INITIATOR, &conn->flags);

	if (!hci_conn_auth(conn, sec_level, auth_type))
		return 0;

encrypt:
	if (test_bit(HCI_CONN_ENCRYPT, &conn->flags))
		return 1;

	hci_conn_encrypt(conn);
	return 0;
}