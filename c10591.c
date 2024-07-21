size_t olm_pk_encryption_set_recipient_key (
    OlmPkEncryption *encryption,
    void const * key, size_t key_length
) {
    if (key_length < olm_pk_key_length()) {
        encryption->last_error =
            OlmErrorCode::OLM_INPUT_BUFFER_TOO_SMALL;
        return std::size_t(-1);
    }
    olm::decode_base64(
        (const uint8_t*)key,
        olm_pk_key_length(),
        (uint8_t *)encryption->recipient_key.public_key
    );
    return 0;
}