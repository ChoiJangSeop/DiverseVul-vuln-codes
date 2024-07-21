TPM2B_ATTEST_Marshal(TPM2B_ATTEST *source, BYTE **buffer, INT32 *size)
{
    UINT16 written = 0;
    written += TPM2B_Marshal(&source->b, buffer, size);
    return written;
}