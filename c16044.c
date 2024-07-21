TPM2B_PUBLIC_KEY_RSA_Marshal(TPM2B_PUBLIC_KEY_RSA *source, BYTE **buffer, INT32 *size)
{
    UINT16 written = 0;
    written += TPM2B_Marshal(&source->b, buffer, size);
    return written;
}