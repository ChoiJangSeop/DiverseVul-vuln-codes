NTSTATUS smb2cli_session_encryption_on(struct smbXcli_session *session)
{
	if (session->smb2->should_encrypt) {
		return NT_STATUS_OK;
	}

	if (session->conn->protocol < PROTOCOL_SMB2_24) {
		return NT_STATUS_NOT_SUPPORTED;
	}

	if (session->conn->smb2.server.cipher == 0) {
		return NT_STATUS_NOT_SUPPORTED;
	}

	if (session->smb2->signing_key.data == NULL) {
		return NT_STATUS_NOT_SUPPORTED;
	}
	session->smb2->should_encrypt = true;
	return NT_STATUS_OK;
}