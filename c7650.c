int hci_conn_check_link_mode(struct hci_conn *conn)
{
	BT_DBG("hcon %p", conn);

	/* In Secure Connections Only mode, it is required that Secure
	 * Connections is used and the link is encrypted with AES-CCM
	 * using a P-256 authenticated combination key.
	 */
	if (hci_dev_test_flag(conn->hdev, HCI_SC_ONLY)) {
		if (!hci_conn_sc_enabled(conn) ||
		    !test_bit(HCI_CONN_AES_CCM, &conn->flags) ||
		    conn->key_type != HCI_LK_AUTH_COMBINATION_P256)
			return 0;
	}

	if (hci_conn_ssp_enabled(conn) &&
	    !test_bit(HCI_CONN_ENCRYPT, &conn->flags))
		return 0;

	/* The minimum encryption key size needs to be enforced by the
	 * host stack before establishing any L2CAP connections. The
	 * specification in theory allows a minimum of 1, but to align
	 * BR/EDR and LE transports, a minimum of 7 is chosen.
	 */
	if (conn->enc_key_size < HCI_MIN_ENC_KEY_SIZE)
		return 0;

	return 1;
}