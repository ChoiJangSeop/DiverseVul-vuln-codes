STACK_OF(PKCS7_SIGNER_INFO) *PKCS7_get_signer_info(PKCS7 *p7)
{
    if (PKCS7_type_is_signed(p7)) {
        return (p7->d.sign->signer_info);
    } else if (PKCS7_type_is_signedAndEnveloped(p7)) {
        return (p7->d.signed_and_enveloped->signer_info);
    } else
        return (NULL);
}