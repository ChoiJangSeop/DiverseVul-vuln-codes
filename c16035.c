TPM2B_ECC_PARAMETER_Marshal(TPM2B_ECC_PARAMETER *source, BYTE **buffer, INT32 *size)
{
    UINT16 written = 0;
    written += TPM2B_Marshal(&source->b, buffer, size);
    return written;
}