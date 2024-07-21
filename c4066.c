bool smbXcli_session_is_guest(struct smbXcli_session *session)
{
	if (session == NULL) {
		return false;
	}

	if (session->conn == NULL) {
		return false;
	}

	if (session->conn->protocol >= PROTOCOL_SMB2_02) {
		if (session->smb2->session_flags & SMB2_SESSION_FLAG_IS_GUEST) {
			return true;
		}
		return false;
	}

	if (session->smb1.action & SMB_SETUP_GUEST) {
		return true;
	}

	return false;
}