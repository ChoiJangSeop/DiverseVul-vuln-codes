TPM2B_CONTEXT_DATA_Marshal(TPM2B_CONTEXT_DATA  *source, BYTE **buffer, INT32 *size)
{
    UINT16 written = 0;
    written += TPM2B_Marshal(&source->b, buffer, size);
    return written;
}