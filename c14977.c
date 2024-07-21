bool AES_GCM_EncryptContext::Encrypt(
	const void *pPlaintextData, size_t cbPlaintextData,
	const void *pIV,
	void *pEncryptedDataAndTag, uint32 *pcbEncryptedDataAndTag,
	const void *pAdditionalAuthenticationData, size_t cbAuthenticationData
) {
    unsigned long long pcbEncryptedDataAndTag_longlong = *pcbEncryptedDataAndTag;

    crypto_aead_aes256gcm_encrypt_afternm(
		static_cast<unsigned char*>( pEncryptedDataAndTag ), &pcbEncryptedDataAndTag_longlong,
		static_cast<const unsigned char*>( pPlaintextData ), cbPlaintextData,
		static_cast<const unsigned char*>(pAdditionalAuthenticationData), cbAuthenticationData,
		nullptr,
		static_cast<const unsigned char*>( pIV ),
		static_cast<const crypto_aead_aes256gcm_state*>( m_ctx )
	);

    *pcbEncryptedDataAndTag = pcbEncryptedDataAndTag_longlong;

    return true;
}