const char *SSL_alert_desc_string_long(int value)
	{
	const char *str;

	switch (value & 0xff)
		{
	case SSL3_AD_CLOSE_NOTIFY:
		str="close notify";
		break;
	case SSL3_AD_UNEXPECTED_MESSAGE:
		str="unexpected_message";
		break;
	case SSL3_AD_BAD_RECORD_MAC:
		str="bad record mac";
		break;
	case SSL3_AD_DECOMPRESSION_FAILURE:
		str="decompression failure";
		break;
	case SSL3_AD_HANDSHAKE_FAILURE:
		str="handshake failure";
		break;
	case SSL3_AD_NO_CERTIFICATE:
		str="no certificate";
		break;
	case SSL3_AD_BAD_CERTIFICATE:
		str="bad certificate";
		break;
	case SSL3_AD_UNSUPPORTED_CERTIFICATE:
		str="unsupported certificate";
		break;
	case SSL3_AD_CERTIFICATE_REVOKED:
		str="certificate revoked";
		break;
	case SSL3_AD_CERTIFICATE_EXPIRED:
		str="certificate expired";
		break;
	case SSL3_AD_CERTIFICATE_UNKNOWN:
		str="certificate unknown";
		break;
	case SSL3_AD_ILLEGAL_PARAMETER:
		str="illegal parameter";
		break;
	case TLS1_AD_DECRYPTION_FAILED:
		str="decryption failed";
		break;
	case TLS1_AD_RECORD_OVERFLOW:
		str="record overflow";
		break;
	case TLS1_AD_UNKNOWN_CA:
		str="unknown CA";
		break;
	case TLS1_AD_ACCESS_DENIED:
		str="access denied";
		break;
	case TLS1_AD_DECODE_ERROR:
		str="decode error";
		break;
	case TLS1_AD_DECRYPT_ERROR:
		str="decrypt error";
		break;
	case TLS1_AD_EXPORT_RESTRICTION:
		str="export restriction";
		break;
	case TLS1_AD_PROTOCOL_VERSION:
		str="protocol version";
		break;
	case TLS1_AD_INSUFFICIENT_SECURITY:
		str="insufficient security";
		break;
	case TLS1_AD_INTERNAL_ERROR:
		str="internal error";
		break;
	case TLS1_AD_USER_CANCELLED:
		str="user canceled";
		break;
	case TLS1_AD_NO_RENEGOTIATION:
		str="no renegotiation";
		break;
	case TLS1_AD_UNSUPPORTED_EXTENSION:
		str="unsupported extension";
		break;
	case TLS1_AD_CERTIFICATE_UNOBTAINABLE:
		str="certificate unobtainable";
		break;
	case TLS1_AD_UNRECOGNIZED_NAME:
		str="unrecognized name";
		break;
	case TLS1_AD_BAD_CERTIFICATE_STATUS_RESPONSE:
		str="bad certificate status response";
		break;
	case TLS1_AD_BAD_CERTIFICATE_HASH_VALUE:
		str="bad certificate hash value";
		break;
	case TLS1_AD_UNKNOWN_PSK_IDENTITY:
		str="unknown PSK identity";
		break;
	default: str="unknown"; break;
		}
	return(str);
	}